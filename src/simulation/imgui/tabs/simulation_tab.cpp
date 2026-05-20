#include "simulation_tab.h"
#include "../../../settings.h"

#include <imgui.h>
#include <cstdio>

void SimulationTab::draw(const SimSnapshot& snap, SimCtx& ctx)
{
    if (m_first_draw_)
    {
        m_thread_count_ = static_cast<int>(PPS_Settings::initial_thread_count);
        m_grid_freq_ = PPS_Settings::add_to_grid_freq;
        m_first_draw_ = false;
    }

    // ══ THREADING ═════════════════════════════════════════════════════════════
    section_header("THREADING");

    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Worker Threads");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::SliderInt("##threads", &m_thread_count_, 1, 64))
        ctx.push({ CommandType::SetThreadCount, {}, 0.f, m_thread_count_ });

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.38f, 0.38f, 0.50f, 1.f });
    ImGui::TextWrapped("Changes take effect next frame. "
        "High counts can increase contention on small particle counts.");
    ImGui::PopStyleColor();

    // ══ SPATIAL GRID ══════════════════════════════════════════════════════════
    section_header("SPATIAL GRID");

    // Grid update frequency (already a non-const int, safe to write directly)
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Update Every N Frames");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::SliderInt("##gridfreq", &m_grid_freq_, 1, 10))
        PPS_Settings::add_to_grid_freq = m_grid_freq_;

    ImGui::Spacing();

    // Read-only grid dimensions (constexpr — displayed for reference)
    auto const_row = [](const char* label, const char* val)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
            ImGui::Text("%-20s", label);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.62f, 0.62f, 0.72f, 1.f });
            ImGui::TextUnformatted(val);
            ImGui::PopStyleColor();
        };

    char buf[32];

    std::snprintf(buf, sizeof(buf), "%zu", PPS_Settings::grid_cells_x);
    const_row("Grid Cells X", buf);

    std::snprintf(buf, sizeof(buf), "%zu", PPS_Settings::grid_cells_y);
    const_row("Grid Cells Y", buf);

    std::snprintf(buf, sizeof(buf), "%d", PPS_Settings::cell_capacity);
    const_row("Cell Capacity", buf);

    thin_sep();

    ImGui::PushStyleColor(ImGuiCol_Text, { 0.38f, 0.38f, 0.50f, 1.f });
    ImGui::TextWrapped("Grid dimensions are compile-time constants. "
        "Rebuild the project to change them in settings.h.");
    ImGui::PopStyleColor();

    // ══ WORLD CONSTANTS ═══════════════════════════════════════════════════════
    section_header("WORLD CONSTANTS");

    std::snprintf(buf, sizeof(buf), "%u", PPS_Settings::particle_count);
    const_row("Particle Count", buf);

    std::snprintf(buf, sizeof(buf), "%.0f", PPS_Settings::visual_radius);
    const_row("Visual Radius", buf);

    std::snprintf(buf, sizeof(buf), "%.1f", PPS_Settings::particle_radius);
    const_row("Particle Radius", buf);

    const float ww = static_cast<float>(PPS_Settings::world_width);
    const float wh = static_cast<float>(PPS_Settings::world_height);
    std::snprintf(buf, sizeof(buf), "%.0f x %.0f", ww, wh);
    const_row("World Size", buf);

    // ══ LIVE PERFORMANCE ══════════════════════════════════════════════════════
    section_header("PERFORMANCE");

    fps_row("Render FPS", snap.stats.fps);
    fps_row("Update FPS", snap.stats.updating_fps);
    stat_row("Sim tick", "%.3f s", snap.sim_tick_seconds);
    stat_row("Iterations", "%d", snap.stats.iterations_);
}