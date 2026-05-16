#pragma once
#include "../../context/sim_snapshot.h"
#include "../../context/sim_command.h"

#include <imgui.h>
struct ITab
{
    virtual ~ITab() = default;
    virtual const char* label() const = 0;
    // snap is READ-ONLY display data
    // toggles is a mutable COPY you can write into freely
    virtual void draw(const SimSnapshot& snap, ImGuiContext& ctx) = 0;

    void toggle(const SimSnapshot& snap, ImGuiContext& ctx, const char* label, bool WorldToggles::* field, const char* shortcut = nullptr)
    {
        bool val = snap.toggles.*field;
        if (ImGui::Checkbox(label, &val))
        {
            WorldToggles new_toggles = snap.toggles;
            new_toggles.*field = val;
            ctx.push({ .type = CommandType::SetToggles, .toggles = new_toggles });
        }
        if (shortcut)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("[%s]", shortcut);
        }
    }


};