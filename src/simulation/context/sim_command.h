#pragma once
#include <mutex>
#include <queue>

#include "../../particle_system/state.h"

enum class CommandType
{
    // ── Toggle state ──────────────────────────────────────────────────────
    SetToggles,             // toggles field carries the full new WorldToggles

    // ── One-shot actions ──────────────────────────────────────────────────
    ResetSimulation,        // Wired: call particle_system_.init_grid_positioning()

    // ── Physics ───────────────────────────────────────────────────────────
    // IMGUI_TODO: in Simulation::resolve_modifications(), handle each:
    //   SetAlpha  → particle_system_.alpha  = cmd.float_val;
    //   SetBeta   → particle_system_.beta   = cmd.float_val;
    //   SetGamma  → particle_system_.gamma  = cmd.float_val;
    //              (requires gamma promoted to non-constexpr in settings.h)
    SetAlpha,
    SetBeta,
    SetGamma,

    // Density grid
    SetDensityCellSize,
    SetGaussianSigma,
    SetBoxFilterCascadePasses,
    SetSinSign,
    SetCosSign,

    // ── World ─────────────────────────────────────────────────────────────
    // IMGUI_TODO: in Simulation::resolve_modifications(), handle each:
    //   RandomizeSimulation → particle_system_.randomize_angles(); 
    //                          particle_system_.init_grid_positioning();
    //   ClearBeacons        → particle_system_.beacons.clear();
    //   SetThreadCount      → rebuild particle_system_.thread_pool with int_val threads
    RandomizeSimulation,
    ClearBeacons,
    SetThreadCount,
};

struct SimCommand
{
    CommandType  type;
    WorldToggles toggles{};
    float        float_val = 0.f;
    int          int_val = 0;
    bool         bool_val = false;
};

// Renamed from ImGuiContext — the original name collides with ImGui's own
// internal ImGuiContext type, causing silent ODR issues with ImGui headers.
struct SimCtx
{
    WorldToggles& toggles;    // mutable copy for this frame — write freely
    std::mutex& cmd_mutex;
    std::queue<SimCommand>& commands;

    void push(SimCommand cmd) const
    {
        std::lock_guard<std::mutex> lock(cmd_mutex);
        commands.push(std::move(cmd));
    }
};