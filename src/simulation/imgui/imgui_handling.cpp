#include "imgui-SFML.h"
#include "../simulation.h"

#include <cstring>   // std::memcmp

void Simulation::handle_imGUI(const SimSnapshot& snap, float dt)
{
    sf::Time delta_time = sf::seconds(dt);
    ImGui::SFML::Update(window_, delta_time);

    // [Q] hides all panels — nothing to draw, bail early
    if (snap.toggles.hide_panels)
        return;

    // Make a mutable copy of the current toggle state.
    // Tabs write into this copy freely; we detect changes below.
    WorldToggles toggles_copy = snap.toggles;
    SimCtx ctx{ toggles_copy, m_cmd_mutex, m_commands };

    m_control_panel_.draw(snap, ctx, dt);

    // If any toggle changed this frame, push a single SetToggles command.
    if (std::memcmp(&toggles_copy, &snap.toggles, sizeof(WorldToggles)) != 0)
        ctx.push({ CommandType::SetToggles, toggles_copy });
}