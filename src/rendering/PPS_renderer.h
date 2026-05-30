#pragma once
#include "heatmap.h"
#include "../settings.h"
#include "../utils/utils.h"
#include "../utils/font.h"
#include "simulation/context/sim_snapshot.h"
#include "utils/Camera.hpp"
#include "../particle_system/density_grid/density_grid_renderer.h"

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

    std::vector<sf::Color> m_cached_colors_;
    bool                   m_colors_dirty_ = true;

    DensityGridRenderer density_grid_renderer{};

public:
    PPS_Renderer(sf::RenderWindow* window);

    static float zoom_to_alpha(const float zoom,
        const float zoom_min, const float zoom_max,
        const float alpha_min, const float alpha_max);

    void render(const SimSnapshot& snapshot, Camera& camera);
    void render_heat_map(const SimSnapshot& snapshot, const Camera& camera, float alpha);
    void render_particles(const SimSnapshot& snapshot, const Camera& camera, float alpha);
    void render_debug(const SimSnapshot& snapshot, const sf::Vector2f mouse_pos, const float mouse_radius);

};