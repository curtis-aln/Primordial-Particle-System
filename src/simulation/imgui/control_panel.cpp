#include "control_panel.h"

#include "tabs/world_tab.h"
#include "tabs/particle_tab.h"
#include "tabs/simulation_tab.h"
#include "tabs/help_tab.h"

#include <imgui.h>

ControlPanel::ControlPanel()
{
    m_tabs_.push_back(std::make_unique<WorldTab>());
    m_tabs_.push_back(std::make_unique<ParticleTab>());
    m_tabs_.push_back(std::make_unique<SimulationTab>());
    m_tabs_.push_back(std::make_unique<HelpTab>());
}

void ControlPanel::draw(const SimSnapshot& snap, SimCtx& ctx, float dt)
{
    constexpr float PANEL_W = 320.f;

    ImGui::SetNextWindowPos({ 10.f, 10.f }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ PANEL_W, 0.f }, ImGuiCond_Always);   // height: auto
    ImGui::SetNextWindowBgAlpha(0.94f);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoSavedSettings;

    // ── Style push ────────────────────────────────────────────────────────────
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 7.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 6.f, 5.f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 11.f, 9.f });
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5.f, 3.f });
    ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 4.f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.07f, 0.07f, 0.10f, 0.95f });
    ImGui::PushStyleColor(ImGuiCol_Border, { 0.20f, 0.20f, 0.28f, 0.60f });
    ImGui::PushStyleColor(ImGuiCol_Tab, { 0.11f, 0.11f, 0.17f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_TabHovered, { 0.22f, 0.30f, 0.45f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_TabActive, { 0.18f, 0.36f, 0.58f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_Separator, { 0.22f, 0.22f, 0.30f, 0.80f });
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, { 0.35f, 0.62f, 0.92f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, { 0.50f, 0.75f, 1.00f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.12f, 0.12f, 0.18f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, { 0.18f, 0.18f, 0.28f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, { 0.22f, 0.22f, 0.34f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_CheckMark, { 0.30f, 0.88f, 0.42f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_Button, { 0.14f, 0.14f, 0.22f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.22f, 0.28f, 0.44f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.28f, 0.45f, 0.70f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_Header, { 0.14f, 0.28f, 0.48f, 0.55f });
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, { 0.20f, 0.36f, 0.58f, 0.80f });
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, { 0.26f, 0.44f, 0.70f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_PopupBg, { 0.09f, 0.09f, 0.13f, 0.97f });

    // ── Window ────────────────────────────────────────────────────────────────
    ImGui::Begin("##pps_ctrl", nullptr, flags);

    // Title bar
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.72f, 0.84f, 1.00f, 1.f });
    ImGui::Text("Primordial Particle System");
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // ── Tab bar ───────────────────────────────────────────────────────────────
    if (ImGui::BeginTabBar("##tabs"))
    {
        for (auto& tab : m_tabs_)
        {
            if (ImGui::BeginTabItem(tab->label()))
            {
                ImGui::Spacing();
                tab->draw(snap, ctx);
                ImGui::Spacing();
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::End();

    // ── Style pop ─────────────────────────────────────────────────────────────
    ImGui::PopStyleColor(19);
    ImGui::PopStyleVar(6);
}