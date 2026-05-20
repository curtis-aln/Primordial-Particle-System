#include "world_tab.h"
#include <imgui.h>
#include <cstdio>

// ── Helpers ───────────────────────────────────────────────────────────────────

static void right_click_radio(SimCtx& ctx, int value, const char* label)
{
    // IMGUI_TODO: right_click_mode is consumed in Simulation::handle_mouse_press().
    //   Check snap.toggles.right_click_mode and act accordingly.
    bool selected = (ctx.toggles.right_click_mode == value);
    if (selected)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.30f, 0.90f, 0.42f, 1.f });
    }
    char id[64];
    std::snprintf(id, sizeof(id), "%s##rcm%d", label, value);
    if (ImGui::RadioButton(id, selected))
        ctx.toggles.right_click_mode = value;
    if (selected) ImGui::PopStyleColor();
}

// ── Tab draw ──────────────────────────────────────────────────────────────────

void WorldTab::draw(const SimSnapshot& snap, SimCtx& ctx)
{
    const auto& stats = snap.stats;

    // ── Header: time elapsed ──────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::Text("Time Elapsed");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.95f, 0.95f, 0.95f, 1.f });
    ImGui::Text("%.2f s", stats.m_total_time_elapsed_);
    ImGui::PopStyleColor();

    // ══ GRAPHICS ══════════════════════════════════════════════════════════════
    section_header("GRAPHICS");

    fps_row("Render FPS", stats.fps);

    ImGui::Spacing();

    // Particle / grid / debug checkboxes in two columns
    float col_w = ImGui::GetContentRegionAvail().x * 0.5f;
    ImGui::BeginGroup();
    toggle(ctx, "Particles", &WorldToggles::render_particles);
    toggle(ctx, "Draw Grid", &WorldToggles::draw_grid);
    ImGui::EndGroup();
    ImGui::SameLine(col_w);
    ImGui::BeginGroup();
    toggle(ctx, "Debug", &WorldToggles::debug_mode, "D");
    ImGui::EndGroup();

    thin_sep();

    // Auto-heatmap + optional manual override
    toggle(ctx, "Auto Heatmap", &WorldToggles::auto_heatmap);
    // IMGUI_TODO: auto_heatmap consumed in PPS_renderer.cpp — PPS_Renderer::render()
    if (!ctx.toggles.auto_heatmap)
    {
        ImGui::Indent(16.f);
        toggle(ctx, "Force Heatmap", &WorldToggles::force_heatmap);
        ImGui::Unindent(16.f);
    }

    toggle(ctx, "Auto Interpolate", &WorldToggles::auto_interpolate);
    // IMGUI_TODO: auto_interpolate consumed in PPS_renderer.cpp — PPS_Renderer::extrapolate_positions()
    if (!ctx.toggles.auto_interpolate)
    {
        ImGui::Indent(16.f);
        toggle(ctx, "Force Interpolate", &WorldToggles::force_interpolate);
        ImGui::Unindent(16.f);
    }

    // ══ STATISTICS ════════════════════════════════════════════════════════════
    section_header("STATISTICS");

    fps_row("Update FPS", stats.updating_fps);

    // Format large integers with comma separators for readability
    {
        char buf[32];
        const int iters = stats.iterations_;
        if (iters >= 1'000'000)
            std::snprintf(buf, sizeof(buf), "%d,%03d,%03d",
                iters / 1'000'000, (iters / 1'000) % 1'000, iters % 1'000);
        else if (iters >= 1'000)
            std::snprintf(buf, sizeof(buf), "%d,%03d", iters / 1'000, iters % 1'000);
        else
            std::snprintf(buf, sizeof(buf), "%d", iters);
        stat_row("Iterations", "%s", buf);
    }
    {
        char buf[32];
        const int cnt = stats.cell_particle_count;
        if (cnt >= 1'000'000)
            std::snprintf(buf, sizeof(buf), "%d,%03d,%03d",
                cnt / 1'000'000, (cnt / 1'000) % 1'000, cnt % 1'000);
        else if (cnt >= 1'000)
            std::snprintf(buf, sizeof(buf), "%d,%03d", cnt / 1'000, cnt % 1'000);
        else
            std::snprintf(buf, sizeof(buf), "%d", cnt);
        stat_row("Particles", "%s", buf);
    }
    stat_row("Sim tick", "%.3f s", snap.sim_tick_seconds);

    // ══ INTERACTION ═══════════════════════════════════════════════════════════
    section_header("INTERACTION");

    // Left-click is always pan — just display it
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Left Click");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.70f, 0.70f, 0.80f, 1.f });
    ImGui::TextUnformatted("Pan");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Right Click");
    ImGui::PopStyleColor();

    ImGui::Indent(10.f);
    right_click_radio(ctx, 0, "Create Cell");
    ImGui::SameLine();
    right_click_radio(ctx, 1, "Destroy");
    ImGui::SameLine();
    right_click_radio(ctx, 2, "Beacon");
    ImGui::Unindent(10.f);

    ImGui::Spacing();

    // Interaction radius slider
    // IMGUI_TODO: interaction_radius consumed in Simulation::handle_mouse_press()
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Radius");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("##radius", &ctx.toggles.interaction_radius, 500.f, 50000.f,
        "%.0f", ImGuiSliderFlags_Logarithmic);

    // Simulation speed slider
    // IMGUI_TODO: sim_speed consumed in Simulation::update()
    //   speed < 1 → skip ticks  |  speed > 1 → multi-step per render frame
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Sim Speed");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("##spd", &ctx.toggles.sim_speed, 0.1f, 8.0f, "%.2fx");

    thin_sep();

    // Action buttons
    const float bw = (ImGui::GetContentRegionAvail().x
        - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

    if (ImGui::Button("Reset", { bw, 0 }))
        ctx.push({ CommandType::ResetSimulation });

    ImGui::SameLine();

    // IMGUI_TODO: RandomizeSimulation — handle in Simulation::resolve_modifications():
    //   particle_system_.randomize_angles();
    //   particle_system_.init_grid_positioning();
    if (ImGui::Button("Randomize", { bw, 0 }))
        ctx.push({ CommandType::RandomizeSimulation });

    // IMGUI_TODO: ClearBeacons — handle in Simulation::resolve_modifications():
    //   particle_system_.beacons.clear();
    if (ImGui::Button("Clear Beacons", { -1.f, 0 }))
        ctx.push({ CommandType::ClearBeacons });

    thin_sep();

    // Read-only quick-key reminder at the bottom
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.32f, 0.32f, 0.42f, 1.f });
    ImGui::TextWrapped("[Space] Pause  [Esc] Quit  [Q] Hide UI");
    ImGui::PopStyleColor();
}