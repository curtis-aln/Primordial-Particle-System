#pragma once
#include <mutex>
#include <queue>

#include "../../particle_system/state.h"

enum class CommandType
{
    // ── Toggle state (whole WorldToggles struct carried in payload) ───────
    SetToggles,

    // ── One-shot actions ──────────────────────────────────────────────────
    ResetSimulation

};

// Only one of these is meaningful per command — think of it like a union but without the hassle
struct SimCommand
{
	CommandType  type; // identifies which of the following fields to read
    

    WorldToggles toggles{};

    float        float_val = 0;
    int          int_val = 0;
    bool         bool_val = false;
};


struct ImGuiContext
{
    WorldToggles& toggles;   // write toggles here freely
    std::mutex& cmd_mutex;
    std::queue<SimCommand>& commands;

    // Helper so tabs don't need to write the lock_guard boilerplate
    void push(SimCommand cmd) const
    {
        std::lock_guard<std::mutex> lock(cmd_mutex);
        commands.push(std::move(cmd));
    }
};