#include "PPS_renderer.h"


inline sf::Texture generate_circle_texture(float radius)
{
    const auto r = static_cast<unsigned>(radius * 2.f);

    sf::RenderTexture renderTexture({ r, r });
    sf::CircleShape circle(radius);

    circle.setFillColor(sf::Color::White);
    circle.setOrigin({ radius, radius });
    circle.setPosition({ radius, radius });
    renderTexture.clear(sf::Color::Transparent);
    renderTexture.draw(circle);
    renderTexture.display();

    sf::Texture tex = renderTexture.getTexture();
    return tex;
}


// colors
inline static constexpr std::uint8_t alpha_col = 200;
inline static constexpr sf::Color Red = { 255, 0,   0,   alpha_col };
inline static constexpr sf::Color Green = { 50,  255, 0,   alpha_col };
inline static constexpr sf::Color Blue = { 0,   0,   255, alpha_col };
inline static constexpr sf::Color Magenta = { 255, 0,   255, alpha_col };
inline static constexpr sf::Color Yellow = { 255, 255, 0,   alpha_col };
inline static constexpr sf::Color Pink = { 255, 192, 203, alpha_col };

// transition thresholds - determined by nearby particles
inline static constexpr float range1 = 22;
inline static constexpr float range2 = 34;
inline static constexpr float range3 = 40;

// colors to be mapped to the ranges
inline static constexpr sf::Color first_color = Green;
inline static constexpr sf::Color second_color = Blue;
inline static constexpr sf::Color third_color = Pink;
inline static constexpr sf::Color fourth_color = Red;

// turning a nearby count into a color which is smoothed
inline sf::Color get_color(const float nearby_count)
{
    if (nearby_count <= 0.0f)
        return first_color;

    if (nearby_count <= range1)
    {
        float factor = nearby_count / range1;
        return {
            static_cast<std::uint8_t>(first_color.r + factor * (second_color.r - first_color.r)),
            static_cast<std::uint8_t>(first_color.g + factor * (second_color.g - first_color.g)),
            static_cast<std::uint8_t>(first_color.b + factor * (second_color.b - first_color.b))
        };
    }
    if (nearby_count <= range2)
    {
        float factor = (nearby_count - range1) / (range2 - range1);
        return {
            static_cast<std::uint8_t>(second_color.r + factor * (third_color.r - second_color.r)),
            static_cast<std::uint8_t>(second_color.g + factor * (third_color.g - second_color.g)),
            static_cast<std::uint8_t>(second_color.b + factor * (third_color.b - second_color.b))
        };
    }
    if (nearby_count < range3)
    {
        float factor = (nearby_count - range2) / (range3 - range2);
        return {
            static_cast<std::uint8_t>(third_color.r + factor * (fourth_color.r - third_color.r)),
            static_cast<std::uint8_t>(third_color.g + factor * (fourth_color.g - third_color.g)),
            static_cast<std::uint8_t>(third_color.b + factor * (fourth_color.b - third_color.b))
        };
    }
    return fourth_color;
}


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

    std::cout << "Renderer Initialized\n";
}

float PPS_Renderer::zoom_to_alpha(const float zoom,
    const float zoom_min, const float zoom_max,
    const float alpha_min, const float alpha_max)
{
    const float t = std::clamp((zoom - zoom_min) / (zoom_max - zoom_min), 0.f, 1.f);
    return alpha_min + t * (alpha_max - alpha_min);
}

void PPS_Renderer::render(const SimSnapshot& snapshot, Camera& camera, bool rend_map, bool rend_particles)
{
    const float left = camera.mapPixelToCoords({ 0, 0 }).x;
    const float right = camera.mapPixelToCoords({ SimulationSettings::screen_width, 0 }).x;
    const float visible_world_width = right - left;

    extrapolate_positions(snapshot, camera, visible_world_width); 

    constexpr float transition_thresh_begin = 800.f * PPS_Settings::particle_radius;
    constexpr float transition_thresh_end = 1400.f * PPS_Settings::particle_radius;
  
    constexpr float diff = transition_thresh_end - transition_thresh_begin;

    const float alpha_heat_map = std::clamp((visible_world_width - transition_thresh_begin) / diff * 255.f, 0.f, 255.f);
    const float alpha_particles = 255.f - alpha_heat_map;

    rend_particles = visible_world_width < transition_thresh_end;
    rend_map = visible_world_width > transition_thresh_begin;

    if (rend_map)       render_heat_map(snapshot, camera, alpha_heat_map);
    if (rend_particles) render_particles(snapshot, camera, alpha_particles);
}

void PPS_Renderer::render_heat_map(const SimSnapshot& snapshot,
    const Camera& camera, const float alpha)
{
    // Select extrapolated positions if available, otherwise read snapshot directly.
    // No copy either way — scatter takes raw pointers.
    const float* pos_x = m_extrapolating_ ? m_extrap_x_.data() : snapshot.render.positions_x.data();
    const float* pos_y = m_extrapolating_ ? m_extrap_y_.data() : snapshot.render.positions_y.data();

    heatmap.clear();
    heatmap.scatter(pos_x, pos_y, PPS_Settings::particle_count, camera.m_view_);
    heatmap.upload();
    heatmap.draw(*window_, static_cast<uint8_t>(alpha));
}

void PPS_Renderer::render_particles(const SimSnapshot& snapshot,
    const Camera& camera, const float alpha)
{
    const auto& neighbourhood_count = snapshot.render.neighbourhood_count_;

    const auto tex_size = static_cast<float>(texture.getSize().x);
    const float u1 = tex_size, v1 = tex_size;

    const sf::Vector2f top_left = camera.mapPixelToCoords({ 0, 0 });
    const sf::Vector2f bottom_right = camera.mapPixelToCoords(
        { SimulationSettings::screen_width, SimulationSettings::screen_height });

    // Select extrapolated positions if available, otherwise read snapshot directly.
    // No copy either way.
    const float* pos_x = m_extrapolating_ ? m_extrap_x_.data() : snapshot.render.positions_x.data();
    const float* pos_y = m_extrapolating_ ? m_extrap_y_.data() : snapshot.render.positions_y.data();

    // Rebuild color cache only when sim produced a new frame (~9fps, not 144fps)
    if (m_colors_dirty_)
    {
        m_cached_colors_.resize(PPS_Settings::particle_count);
        for (size_t i = 0; i < PPS_Settings::particle_count; ++i)
            m_cached_colors_[i] = get_color(neighbourhood_count[i]);
        m_colors_dirty_ = false;
    }

    vertex_array.clear();

    for (size_t i = 0; i < PPS_Settings::particle_count; ++i)
    {
        const float px_v = pos_x[i], py_v = pos_y[i];
        constexpr float r = PPS_Settings::particle_radius;

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

    window_->draw(vertex_array, states);
}

void PPS_Renderer::render_debug(const SimSnapshot& snapshot,
    const sf::Vector2f mouse_pos, const float mouse_radius)
{
    const auto* positions_x = &snapshot.render.positions_x;
    const auto* positions_y = &snapshot.render.positions_y;
    const auto* neighbourhood_count = &snapshot.render.neighbourhood_count_;
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

        sf::Vector2f direction = sf::Vector2f(std::sin(angle), std::cos(angle)) * PPS_Settings::particle_radius;
        debug_lines_[i * 2].position = position;
        debug_lines_[i * 2].color = sf::Color::White;
        debug_lines_[i * 2 + 1].position = position + direction;
        debug_lines_[i * 2 + 1].color = sf::Color::White;

        visual_radius_shape_.setPosition(position - sf::Vector2f(visual_radius, visual_radius));
        window_->draw(visual_radius_shape_, sf::BlendAdd);
    }

    window_->draw(debug_lines_);
}


// Called from the RENDER THREAD after begin_read() sees a new snapshot.
// Never call this from the sim/update thread.
void PPS_Renderer::notify_new_snapshot(const SimSnapshot& snapshot)
{
    m_sim_tick_seconds_ = snapshot.sim_tick_seconds;
    m_snapshot_age_.restart();
    m_colors_dirty_ = true;   // add this
}

void PPS_Renderer::extrapolate_positions(const SimSnapshot& snapshot,
    const Camera& camera,
    const float visible_world_width)
{
    constexpr float transition_thresh_begin = 800.f * PPS_Settings::particle_radius;
    constexpr float extrap_disable_width = transition_thresh_begin * 0.25f; // ~200*radius

    constexpr float enable_threshold_seconds = 1.f / 20.f;

    m_extrapolating_ = (visible_world_width <= extrap_disable_width)
        && (m_sim_tick_seconds_ > enable_threshold_seconds);

    if (!m_extrapolating_) return;

    const int   n = PPS_Settings::particle_count;
    const float gamma = PPS_Settings::gamma;

    if (static_cast<int>(m_extrap_x_.size()) != n) { m_extrap_x_.resize(n); m_extrap_y_.resize(n); }

    // ── Visible frustum (with one-particle margin) ────────────────────────
    constexpr float r = PPS_Settings::particle_radius;
    const sf::Vector2f tl = camera.mapPixelToCoords({ 0, 0 });
    const sf::Vector2f br = camera.mapPixelToCoords(
        { SimulationSettings::screen_width, SimulationSettings::screen_height });
    const float min_x = tl.x - r, max_x = br.x + r;
    const float min_y = tl.y - r, max_y = br.y + r;

    // Remove the smoothstep, replace with:

    const float elapsed = m_snapshot_age_.getElapsedTime().asSeconds();
    const float t = std::clamp(elapsed / m_sim_tick_seconds_, 0.f, 1.f);

    const float* ca = snapshot.render.cos_angles_.data();
    const float* sa = snapshot.render.sin_angles_.data();
    const float* px = snapshot.render.positions_x.data();
    const float* py = snapshot.render.positions_y.data();
    float* ex = m_extrap_x_.data();
    float* ey = m_extrap_y_.data();

    for (int i = 0; i < n; ++i)
    {
        const float x = px[i], y = py[i];
        if (x < min_x || x > max_x || y < min_y || y > max_y)
        {
            ex[i] = x; ey[i] = y;
            continue;
        }

        // Per-particle variation: cheap integer hash → float in [0, 1]
        const float variation = static_cast<float>((i * 2654435761u) & 0xFFFF) / 65535.f;

        // blend in [0.3, 0.7]: controls how much it decelerates
        // t*(1 - blend*(1-t)) goes 0→1, speed starts at 1, ends at (1-blend)
        // so speed floor is 0.3 to 0.7 — never hits zero, all different
        const float blend = 0.3f + 0.4f * variation;
        const float t_eased = t * (1.f - blend * (1.f - t));

        ex[i] = x + ca[i] * gamma * t_eased;
        ey[i] = y + sa[i] * gamma * t_eased;
    }
}