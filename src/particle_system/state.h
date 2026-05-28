#pragma once
#include <SFML/Graphics/Color.hpp>
#include <vector>

#include "../settings.h"
#include "density_grid/density_grid.h"

// ─────────────────────────────────────────────────────────────────────────────
//  WorldToggles
//  Owned by the main thread (ImGui writes, update thread reads).
//  Copied into SimSnapshot each frame before the update thread reads it.
// ─────────────────────────────────────────────────────────────────────────────
struct WorldToggles
{
    // ── Existing ─────────────────────────────────────────────────────────────
    bool debug_mode = false;
    bool paused = true;
    bool draw_grid = false;
    bool track_statistics = true;
    bool m_tick_frame_time = false;
    bool m_rendering_ = true;
    bool hide_panels = false;
    bool render_particles = true;

    bool use_density_grid = true;

    // ── Rendering mode ────────────────────────────────────────────────────────
    // IMGUI_TODO: consume in PPS_renderer.cpp — PPS_Renderer::render()
    //   if  auto_heatmap → existing zoom-based switching (current behaviour)
    //   else             → use force_heatmap value directly
    bool auto_heatmap = true;
    bool force_heatmap = false;

    // IMGUI_TODO: consume in PPS_renderer.cpp — PPS_Renderer::extrapolate_positions()
    //   if  auto_interpolate → existing tick-rate-based enabling (current behaviour)
    //   else                 → use force_interpolate value directly
    bool auto_interpolate = true;
    bool force_interpolate = false;

    // ── Right-click interaction ────────────────────────────────────────────────
    // IMGUI_TODO: consume in Simulation::handle_mouse_press() / dispatch_event()
    //   0 = create cell  (calls particle_system_.create_cell_at(pos, n))
    //   1 = destroy      (zero out particles within radius — not yet implemented)
    //   2 = place beacon (calls particle_system_.beacons.add(pos))
    int   right_click_mode = 0;

    // IMGUI_TODO: consume in Simulation::handle_mouse_press() as the effect radius
    float interaction_radius = 8000.f;

    // ── Simulation speed ──────────────────────────────────────────────────────
    // IMGUI_TODO: consume in Simulation::update()
    //   speed < 1 → skip ticks:   run update only when (frame % round(1/speed) == 0)
    //   speed > 1 → multi-step:   run update round(speed) times per render frame
    float sim_speed = 1.0f;


    bool box_smoothing_ = true; // button
    bool gaussian_smoothing_ = false; // button
};

// ─────────────────────────────────────────────────────────────────────────────
//  WorldStatistics — written by update thread, read via snapshot
// ─────────────────────────────────────────────────────────────────────────────
struct WorldStatistics
{
    int   nutrient_count = 0;
    int   spore_particle_count = 0;
    int   cell_particle_count = 0;

    float fps = 0.f;
    float updating_fps = 0.f;
    int   iterations_ = 0;

    float m_total_time_elapsed_ = 0.f;
};

// ─────────────────────────────────────────────────────────────────────────────
//  RenderData — written by update thread, read by render thread via snapshot
// ─────────────────────────────────────────────────────────────────────────────
struct RenderData
{
    alignas(64) std::vector<float>    positions_x{};
    alignas(64) std::vector<float>    positions_y{};
    alignas(64) std::vector<float>    angles_{};
    alignas(64) std::vector<float>    cos_angles_{};
    alignas(64) std::vector<float>    sin_angles_{};
    alignas(64) std::vector<uint16_t> neighbourhood_count_{};
    alignas(64) std::vector<sf::Color> colors{};

    RenderData()
    {
        constexpr int n = PPS_Settings::particle_count;
        positions_x.resize(n);
        positions_y.resize(n);
        colors.resize(n);
        angles_.resize(n);
        cos_angles_.resize(n);
        sin_angles_.resize(n);
        neighbourhood_count_.resize(n);
    }
};