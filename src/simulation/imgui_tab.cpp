#include "imgui-SFML.h"
#include "simulation.h"

void Simulation::handle_imGUI(const SimSnapshot& snap, float dt)
{

    sf::Time delta_time = sf::seconds(static_cast<float>(dt));
    ImGui::SFML::Update(window_, delta_time);

    draw_tab(snap, dt);
}

void Simulation::draw_tab(const SimSnapshot& snap, float dt)
{
    const auto& stats = snap.stats;
    const auto& toggles = particle_system_.toggles;

    constexpr float PANEL_WIDTH = 300.f;

    ImGui::SetNextWindowPos({ 10.f, 10.f }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ PANEL_WIDTH, 0.f }, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.f, 6.f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 14.f, 12.f });

    ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.08f, 0.08f, 0.10f, 0.90f });
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 0.13f, 0.13f, 0.18f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_Separator, { 0.30f, 0.30f, 0.35f, 0.80f });

    if (ImGui::Begin("  Primordial Particle System", nullptr, flags))
    {
        // ── Description ───────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.55f, 0.55f, 0.65f, 1.f });
        ImGui::TextWrapped("An emergent self-organising particle simulation.");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ── Performance ───────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.85f, 0.75f, 0.40f, 1.f });
        ImGui::Text("  Performance");
        ImGui::PopStyleColor();

        ImGui::Spacing();

        auto stat_row = [](const char* label, const char* fmt, ...)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, { 0.55f, 0.55f, 0.65f, 1.f });
                ImGui::Text("%-18s", label);
                ImGui::PopStyleColor();
                ImGui::SameLine();

                char buf[64];
                va_list args;
                va_start(args, fmt);
                vsnprintf(buf, sizeof(buf), fmt, args);
                va_end(args);

                ImGui::PushStyleColor(ImGuiCol_Text, { 0.95f, 0.95f, 0.95f, 1.f });
                ImGui::Text("%s", buf);
                ImGui::PopStyleColor();
            };

        const float render_fps = stats.fps;
        const float update_fps = stats.updating_fps;

        // colour-code FPS: green > 55, yellow > 30, red otherwise
        auto fps_color = [](float f) -> ImVec4 {
            if (f >= 55.f) return { 0.35f, 0.90f, 0.45f, 1.f };
            if (f >= 30.f) return { 0.95f, 0.80f, 0.20f, 1.f };
            return               { 0.95f, 0.30f, 0.25f, 1.f };
            };

        ImGui::PushStyleColor(ImGuiCol_Text, { 0.55f, 0.55f, 0.65f, 1.f });
        ImGui::Text("%-18s", "Render FPS");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, fps_color(render_fps));
        ImGui::Text("%.1f", render_fps);
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, { 0.55f, 0.55f, 0.65f, 1.f });
        ImGui::Text("%-18s", "Update FPS");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, fps_color(update_fps));
        ImGui::Text("%.1f", update_fps);
        ImGui::PopStyleColor();

        stat_row("Particles", "%d", stats.cell_particle_count);
        stat_row("Iterations", "%d", stats.iterations_);
        stat_row("Time Elapsed", "%.2f s", stats.m_total_time_elapsed_);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ── Toggles ───────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.85f, 0.75f, 0.40f, 1.f });
        ImGui::Text("  Toggles");
        ImGui::PopStyleColor();

        ImGui::Spacing();

        struct Toggle {
            const char* label;
            const char* key;
            bool        state;
        };

        const Toggle toggles_list[] = {
            { "Paused",       "[Space]", toggles.paused           },
            { "Debug",        "[D]",     toggles.debug_mode       },
            { "Rendering",    "[R]",     toggles.m_rendering_     },
            { "Hide Panels",  "[Q]",     toggles.hide_panels      },
            { "Draw Grid",    "[ ]",     toggles.draw_grid        },
            { "Statistics",   "[ ]",     toggles.track_statistics },
            { "Tick Frame",   "[O]",     toggles.m_tick_frame_time},
        };

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.f, 5.f });

        for (const auto& t : toggles_list)
        {
            // Indicator dot
            ImVec4 dot_col = t.state
                ? ImVec4{ 0.35f, 0.90f, 0.45f, 1.f }
            : ImVec4{ 0.45f, 0.45f, 0.50f, 1.f };

            ImGui::PushStyleColor(ImGuiCol_Text, dot_col);
            ImGui::Text(t.state ? "  " : "  ");  // spacing placeholder
            ImGui::SameLine(0.f, 0.f);
            ImGui::Text("%s", t.state ? "\xe2\x97\x8f" : "\xe2\x97\x8b"); // ● / ○
            ImGui::PopStyleColor();

            ImGui::SameLine(30.f);
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.90f, 0.90f, 0.90f, 1.f });
            ImGui::Text("%-16s", t.label);
            ImGui::PopStyleColor();

            ImGui::SameLine(PANEL_WIDTH - 60.f);
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.45f, 0.55f, 0.75f, 1.f });
            ImGui::Text("%s", t.key);
            ImGui::PopStyleColor();
        }

        ImGui::PopStyleVar(); // ItemSpacing (toggles)

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, { 0.35f, 0.35f, 0.40f, 1.f });
        ImGui::Text("[Esc] Quit   [O] Step frame   [Scroll] Zoom");
        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleColor(3); // WindowBg, TitleBgActive, Separator
    ImGui::PopStyleVar(4);   // WindowRounding, FrameRounding, ItemSpacing, WindowPadding
}