#pragma once
#include "heatmap.h"
#include "../settings.h"
#include "../utils/utils.h"
#include "../utils/font.h"
#include "simulation/context/sim_snapshot.h"
#include "utils/Camera.hpp"

struct SimSnapshot;

class PPS_Renderer
{
    sf::Texture texture;
    sf::VertexArray vertex_array{ sf::PrimitiveType::Triangles };

    sf::RenderStates states{};

    // Debug rendering
    sf::CircleShape visual_radius_shape_;
    sf::VertexArray debug_lines_;

    sf::RenderWindow* window_ = nullptr;

    DensityHeatmap heatmap{ PPS_Settings::world_width, PPS_Settings::world_height,
                             SimulationSettings::screen_width, SimulationSettings::screen_height };

    // ── Extrapolation ──────────────────────────────────────────────────────
    sf::Clock          m_snapshot_age_;         // reset each time a new snapshot arrives (render thread only)
    float              m_sim_tick_seconds_ = 0.f;
    bool               m_extrapolating_ = false; // true when sim fps < threshold
    std::vector<float> m_extrap_x_;
    std::vector<float> m_extrap_y_;

    std::vector<sf::Color> m_cached_colors_;
    bool                   m_colors_dirty_ = true;

public:
    PPS_Renderer(sf::RenderWindow* window);

    static float zoom_to_alpha(const float zoom,
        const float zoom_min, const float zoom_max,
        const float alpha_min, const float alpha_max);

    void render(const SimSnapshot& snapshot, Camera& camera, bool rend_map = true, bool rend_particles = true);
    void render_heat_map(const SimSnapshot& snapshot, const Camera& camera, float alpha);
    void render_particles(const SimSnapshot& snapshot, const Camera& camera, float alpha);
    void render_debug(const SimSnapshot& snapshot, const sf::Vector2f mouse_pos, const float mouse_radius);

    // Call from the render thread immediately after begin_read() detects a new snapshot.
    // Must NOT be called from the sim/update thread.
    void notify_new_snapshot(const SimSnapshot& snapshot);

    void extrapolate_positions(const SimSnapshot& snapshot, const Camera& camera, float visible_world_width);
};