#include "control_panel.h"

#include "tabs/simulation_tab.h"
#include "tabs/world_tab.h"
#include "tabs/help_tab.h"
#include "tabs/particle_tab.h"

#include <imgui.h>

ControlPanel::ControlPanel()
{
    //m_tabs_.push_back(std::make_unique<SimulationTab>());
    //m_tabs_.push_back(std::make_unique<StatisticsTab>());
    //m_tabs_.push_back(std::make_unique<GraphsTab>());
    //m_tabs_.push_back(std::make_unique<OrganismTab>());
}


void ControlPanel::draw(const SimSnapshot& snap, ImGuiContext& ctx, float dt)
{
    ImGui::SetNextWindowPos({ 10.f, 10.f }, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({ 520.f, 640.f }, ImGuiCond_FirstUseEver);

    ImGui::Begin("ARIA Control Panel", nullptr, ImGuiWindowFlags_NoNav);

    if (ImGui::BeginTabBar("##ctrl_tabs"))
    {
        for (auto& tab : m_tabs_)
        {
            if (ImGui::BeginTabItem(tab->label()))
            {
                tab->draw(snap, ctx);
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}