#pragma once
#include <SFML/Graphics.hpp>

struct SimulationSettings
{
    static constexpr bool set_Random_seed = false;
    inline static constexpr unsigned screen_width = 1900;
    inline static constexpr unsigned screen_height = 1000;
    inline static constexpr auto aspect_ratio =
        static_cast<float>(screen_width) / static_cast<float>(screen_height);

    inline static constexpr unsigned max_frame_rate = 5200;
    inline static const sf::Color    screen_color = { 0, 0, 0 };
    inline static const std::string  simulation_title = "Primordial Particle Simulation";

    inline static constexpr bool Vsync = false;
};

struct PPS_Settings
{
    inline static constexpr unsigned threads = 16;
    inline static constexpr unsigned particle_count = 1'000'000;

    inline static int add_to_grid_freq = 3;

    inline static constexpr float scale_factor = 900;
    inline static constexpr float param_scale_factor = 200.f;

    inline static constexpr auto world_width = SimulationSettings::screen_width * scale_factor;
    inline static constexpr auto world_height = SimulationSettings::screen_height * scale_factor;

    inline static constexpr auto grid_cells_y =
        static_cast<size_t>(scale_factor);
    inline static constexpr auto grid_cells_x =
        static_cast<size_t>((scale_factor)*SimulationSettings::aspect_ratio);
    inline static constexpr int cell_capacity = 20;

    // Scale-sensitive parameters
    inline static constexpr float visual_radius = 5.f * param_scale_factor;

    // IMGUI_TODO: gamma was constexpr; promoted to runtime-editable so the
    //   Particle tab slider can write to it.  Any constexpr use-site that
    //   depended on it will need to be updated (none found at time of change).
    inline static float gamma = 0.67f * param_scale_factor;

    // Main simulation rules — already non-constexpr, safe to write from GUI
    inline static float alpha = 180.f;
    inline static float beta = 17.f;

    // Graphical settings
    inline static float particle_radius = 160.f;

    inline static constexpr size_t max_beacon_count = 100;
    inline static constexpr float  init_position_scatter = 125.f;
};