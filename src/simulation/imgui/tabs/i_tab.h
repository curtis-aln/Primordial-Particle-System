#pragma once
#include "../../context/sim_snapshot.h"
#include "../../context/sim_command.h"

#include <imgui.h>
#include <cstdarg>
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
//  ITab — abstract base for all control-panel tabs
// ─────────────────────────────────────────────────────────────────────────────
struct ITab
{
    virtual ~ITab() = default;
    virtual const char* label() const = 0;
    virtual void draw(const SimSnapshot& snap, SimCtx& ctx) = 0;

    // ── Shared helpers ────────────────────────────────────────────────────────

    // Amber section header with a tinted separator line underneath.
    static void section_header(const char* text)
    {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.88f, 0.74f, 0.32f, 1.f });
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Separator, { 0.88f, 0.74f, 0.32f, 0.28f });
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    // Two-column stat row: muted label left, bright value right.
    static void stat_row(const char* label, const char* fmt, ...)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
        ImGui::Text("%-18s", label);
        ImGui::PopStyleColor();
        ImGui::SameLine();

        char buf[64];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        ImGui::PushStyleColor(ImGuiCol_Text, { 0.95f, 0.95f, 0.95f, 1.f });
        ImGui::TextUnformatted(buf);
        ImGui::PopStyleColor();
    }

    // FPS value: green ≥55, yellow ≥30, red otherwise.
    static ImVec4 fps_color(float fps)
    {
        if (fps >= 55.f) return { 0.28f, 0.90f, 0.40f, 1.f };
        if (fps >= 30.f) return { 0.95f, 0.80f, 0.18f, 1.f };
        return                  { 0.95f, 0.28f, 0.22f, 1.f };
    }

    // FPS row with colour coding.
    static void fps_row(const char* label, float fps)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
        ImGui::Text("%-18s", label);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, fps_color(fps));
        ImGui::Text("%.1f", fps);
        ImGui::PopStyleColor();
    }

    // Checkbox bound directly to a bool member of ctx.toggles.
    // Writes into the mutable copy — imgui_handling.cpp pushes SetToggles on change.
    static void toggle(SimCtx& ctx, const char* label,
        bool WorldToggles::* field,
        const char* shortcut = nullptr)
    {
        bool& val = ctx.toggles.*field;
        ImGui::Checkbox(label, &val);
        if (shortcut)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.38f, 0.48f, 0.70f, 1.f });
            ImGui::Text("[%s]", shortcut);
            ImGui::PopStyleColor();
        }
    }

    // Indicator dot (●/○) + label row, no interaction.
    static void indicator_row(bool active, const char* label, const char* key = nullptr)
    {
        const ImVec4 dot_col = active
            ? ImVec4{ 0.28f, 0.90f, 0.40f, 1.f }
        : ImVec4{ 0.38f, 0.38f, 0.46f, 1.f };

        ImGui::PushStyleColor(ImGuiCol_Text, dot_col);
        ImGui::TextUnformatted(active ? "\xe2\x97\x8f" : "\xe2\x97\x8b"); // ● / ○
        ImGui::PopStyleColor();

        ImGui::SameLine(22.f);
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.88f, 0.88f, 0.88f, 1.f });
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();

        if (key)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.38f, 0.48f, 0.70f, 1.f });
            ImGui::Text("[%s]", key);
            ImGui::PopStyleColor();
        }
    }

    // Thin full-width separator with a little vertical breathing room.
    static void thin_sep()
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
};