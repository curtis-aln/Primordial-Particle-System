#pragma once
#include <SFML/Graphics/Color.hpp>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
//  WorldToggles
//  Owned by the main thread (ImGui writes, update thread reads).
//  Copied into SharedState each frame before the update thread reads it.
// ─────────────────────────────────────────────────────────────────────────────
struct WorldToggles
{
    bool debug_mode = false;  // show per-cell debug info
    bool paused = false;  // pause the simulation update loop
    bool draw_grid = false;  // render the cell spatial hash grid
    bool track_statistics = true;   // gather per-frame statistics
    bool m_tick_frame_time = false;  // whether to advance the simulation by one tick (for debugging)
    bool  m_rendering_ = true; // whether to render the simulation (for debugging)
    bool  hide_panels = false; // whether to hide ImGui panels (for recording clean screenshots)
};

// ─────────────────────────────────────────────────────────────────────────────
//  WorldStatistics
//  Owned by the update thread (sim writes every tick).
//  Copied into the snapshot so ImGui can read it safely.
// ─────────────────────────────────────────────────────────────────────────────
struct WorldStatistics
{
    int nutrient_count = 0;
    int spore_particle_count = 0;
    int cell_particle_count = 0;

    float fps = 0.f;
    float updating_fps = 0.f;
    int iterations_ = 0;

    float m_total_time_elapsed_ = 0.f;
};

// ─────────────────────────────────────────────────────────────────────────────
//  RenderData
//  Written by the update thread, read by the render thread.
//  Contains everything needed to draw the simulation without touching
//  live simulation objects.
// ─────────────────────────────────────────────────────────────────────────────
struct RenderData
{
    alignas(64) std::vector<float> positions_x{};
    alignas(64) std::vector<float> positions_y{};
    alignas(64) std::vector<float> angles_{};

    alignas(64) std::vector<uint16_t> neighbourhood_count_{};
    alignas(64) std::vector<sf::Color> colors{}; // filled based on neigbhourhood count, happens on the render thread

    RenderData()
    {
        constexpr int n = PPS_Settings::particle_count;
        positions_x.resize(n);
        positions_y.resize(n);
        colors.resize(n);
        angles_.resize(n);
        neighbourhood_count_.resize(n);
    }
};
