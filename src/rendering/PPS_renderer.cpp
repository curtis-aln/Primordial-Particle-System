#include "PPS_renderer.h"
#include "particle_colors.h"

// ── Global color scheme (definition) ──────────────────────────────────────────
// Declared extern in particle_colors.h; defined here (render thread only).
ParticleColorScheme g_color_scheme{};
bool                g_color_scheme_dirty = true;

// ── Circle texture ────────────────────────────────────────────────────────────
inline sf::Texture generate_circle_texture(float radius)
{
    const auto r = static_cast<unsigned>(radius * 2.f);

    sf::RenderTexture renderTexture({ r, r });
    sf::CircleShape   circle(radius);

    circle.setFillColor(sf::Color::White);
    circle.setOrigin({ radius, radius });
    circle.setPosition({ radius, radius });
    renderTexture.clear(sf::Color::Transparent);
    renderTexture.draw(circle);
    renderTexture.display();

    return renderTexture.getTexture();
}

// ── Constructor ───────────────────────────────────────────────────────────────
PPS_Renderer::PPS_Renderer(sf::RenderWindow* window) : window_(window)
{
    constexpr int vertex_count = PPS_Settings::particle_count * 6;
    vertex_array.resize(vertex_count);

    visual_radius_shape_.setFillColor(sf::Color::Transparent);
    visual_radius_shape_.setOutlineThickness(5);
    visual_radius_shape_.setOutlineColor(sf::Color(255, 255, 255, 100));
    visual_radius_shape_.setRadius(PPS_Settings::visual_radius);

    debug_lines_.setPrimitiveType(sf::PrimitiveType::Lines);

    texture = generate_circle_texture(PPS_Settings::particle_radius);
    texture.setSmooth(true);

    states.blendMode = sf::BlendAdd;

    heatmap.set_trail_decay(.45);

    std::cout << "Renderer Initialized\n";
}

// ── Zoom → alpha helper ───────────────────────────────────────────────────────
float PPS_Renderer::zoom_to_alpha(const float zoom,
    const float zoom_min, const float zoom_max,
    const float alpha_min, const float alpha_max)
{
    const float t = std::clamp((zoom - zoom_min) / (zoom_max - zoom_min), 0.f, 1.f);
    return alpha_min + t * (alpha_max - alpha_min);
}

// ── Main render ───────────────────────────────────────────────────────────────
void PPS_Renderer::render(const SimSnapshot& snapshot, Camera& camera)
{
    bool rend_particles = snapshot.toggles.render_particles;
    bool rend_map = true;

    const float left = camera.mapPixelToCoords({ 0, 0 }).x;
    const float right = camera.mapPixelToCoords(
        { SimulationSettings::screen_width, 0 }).x;
    const float visible_world_width = right - left;

    const float transition_thresh_begin = 800.f * PPS_Settings::particle_radius;
    const float transition_thresh_end = 1400.f * PPS_Settings::particle_radius;
    const float diff = transition_thresh_end - transition_thresh_begin;

    const auto& tgl = snapshot.toggles;

    float alpha_heat_map;
    float alpha_particles;

    if (tgl.auto_heatmap)
    {
        // Original zoom-based blending — both alphas derived from zoom level
        alpha_heat_map = std::clamp(
            (visible_world_width - transition_thresh_begin) / diff * 255.f,
            0.f, 255.f);
        alpha_particles = 255.f - alpha_heat_map;

        //if (rend_particles != false)
        rend_particles = visible_world_width < transition_thresh_end;

        rend_map = visible_world_width > transition_thresh_begin;
    }
    else
    {
        // Manual override — each mode is fully on or fully off, independently
        rend_map = tgl.force_heatmap;
        rend_particles = tgl.render_particles;

        alpha_heat_map = rend_map ? 255.f : 0.f;
        alpha_particles = rend_particles ? 255.f : 0.f;
    }

    if (rend_map)       render_heat_map(snapshot, camera, alpha_heat_map);
    if (rend_particles) render_particles(snapshot, camera, alpha_particles);
}


// ── Heat map ──────────────────────────────────────────────────────────────────
void PPS_Renderer::render_heat_map(const SimSnapshot& snapshot,
    const Camera& camera, const float alpha)
{

    heatmap.clear();
    heatmap.scatter(snapshot.render.positions_x.data(), snapshot.render.positions_y.data(), PPS_Settings::particle_count, camera.m_view_);
    heatmap.upload();
    heatmap.draw(*window_, static_cast<uint8_t>(alpha));
}

// ── Particle rendering ────────────────────────────────────────────────────────
void PPS_Renderer::render_particles(const SimSnapshot& snapshot,
    const Camera& camera, const float alpha)
{
    const auto& neighbourhood_count = snapshot.render.neighbourhood_count_;

    const auto  tex_size = static_cast<float>(texture.getSize().x);
    const float u1 = tex_size, v1 = tex_size;

    const sf::Vector2f top_left = camera.mapPixelToCoords({ 0, 0 });
    const sf::Vector2f bottom_right = camera.mapPixelToCoords(
        { SimulationSettings::screen_width, SimulationSettings::screen_height });

 
    // Rebuild color cache when sim produced a new frame OR color scheme changed
    if (m_colors_dirty_ || g_color_scheme_dirty)
    {
        m_cached_colors_.resize(PPS_Settings::particle_count);
        for (size_t i = 0; i < PPS_Settings::particle_count; ++i)
            m_cached_colors_[i] = interpolate_scheme(
                static_cast<float>(neighbourhood_count[i]), g_color_scheme);

        m_colors_dirty_ = false;
        g_color_scheme_dirty = false;
    }

    vertex_array.clear();

    for (size_t i = 0; i < PPS_Settings::particle_count; ++i)
    {
        const float px_v = snapshot.render.positions_x[i], py_v = snapshot.render.positions_y[i];
        const float r = PPS_Settings::particle_radius;

        if (px_v < top_left.x || py_v < top_left.y ||
            px_v > bottom_right.x || py_v > bottom_right.y)
            continue;

        sf::Color col = m_cached_colors_[i];
        col.a = static_cast<uint8_t>(alpha);

        vertex_array.append({ .position = { px_v - r, py_v - r }, .color = col, .texCoords = { 0.f, 0.f } });
        vertex_array.append({ .position = { px_v + r, py_v - r }, .color = col, .texCoords = { u1,  0.f } });
        vertex_array.append({ .position = { px_v + r, py_v + r }, .color = col, .texCoords = { u1,  v1  } });
        vertex_array.append({ .position = { px_v - r, py_v - r }, .color = col, .texCoords = { 0.f, 0.f } });
        vertex_array.append({ .position = { px_v + r, py_v + r }, .color = col, .texCoords = { u1,  v1  } });
        vertex_array.append({ .position = { px_v - r, py_v + r }, .color = col, .texCoords = { 0.f, v1  } });
    }

    states.texture = &texture;
    window_->draw(vertex_array, states);
}

// ── Debug overlay ─────────────────────────────────────────────────────────────
void PPS_Renderer::render_debug(const SimSnapshot& snapshot,
    const sf::Vector2f mouse_pos, const float mouse_radius)
{
    const auto* positions_x = &snapshot.render.positions_x;
    const auto* positions_y = &snapshot.render.positions_y;
    const auto* angles = &snapshot.render.angles_;

    constexpr float visual_radius = PPS_Settings::visual_radius;

    debug_lines_.clear();
    debug_lines_.resize(positions_x->size() * 2);

    for (size_t i = 0; i < positions_x->size(); ++i)
    {
        const sf::Vector2f position = { (*positions_x)[i], (*positions_y)[i] };

        if (dist_squared(position, mouse_pos) > mouse_radius * mouse_radius)
            continue;

        const float angle = (*angles)[i];
        const sf::Vector2f direction =
            sf::Vector2f(std::sin(angle), std::cos(angle)) * PPS_Settings::particle_radius;

        debug_lines_[i * 2].position = position;
        debug_lines_[i * 2].color = sf::Color::White;
        debug_lines_[i * 2 + 1].position = position + direction;
        debug_lines_[i * 2 + 1].color = sf::Color::White;

        visual_radius_shape_.setPosition(position - sf::Vector2f(visual_radius, visual_radius));
        window_->draw(visual_radius_shape_, sf::BlendAdd);
    }

    window_->draw(debug_lines_);
}
