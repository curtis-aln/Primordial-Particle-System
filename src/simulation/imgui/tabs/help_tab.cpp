#include "help_tab.h"
#include <imgui.h>

// ── Helper — one keybind row ───────────────────────────────────────────────────
static void keybind(const char* key, const char* action)
{
    // Key badge
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.75f, 0.85f, 1.00f, 1.f });
    ImGui::PushStyleColor(ImGuiCol_Button, { 0.14f, 0.20f, 0.34f, 1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.14f, 0.20f, 0.34f, 1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.14f, 0.20f, 0.34f, 1.f });

    // Use a small button as a non-interactive key "badge"
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.f, 1.f });
    ImGui::SmallButton(key);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    ImGui::SameLine(0.f, 8.f);
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.82f, 0.82f, 0.90f, 1.f });
    ImGui::TextUnformatted(action);
    ImGui::PopStyleColor();
}

void HelpTab::draw(const SimSnapshot& /*snap*/, SimCtx& /*ctx*/)
{
    // ══ KEYBINDS ══════════════════════════════════════════════════════════════
    section_header("KEYBINDS");

    keybind("Space", "Pause / Resume simulation");
    keybind("Esc", "Quit");
    keybind("D", "Toggle debug overlay");
    keybind("R", "Toggle particle rendering");
    keybind("Q", "Hide / show UI panels");
    keybind("O", "Step one frame (while paused)");
    keybind("Scroll", "Zoom in / out");
    keybind("LMB Drag", "Pan camera");
    keybind("RMB Click", "Apply selected interaction");

    thin_sep();

    // ══ HOW IT WORKS ══════════════════════════════════════════════════════════
    section_header("HOW IT WORKS");

    // Use a child region so the explanation can scroll independently
    // if the panel is shorter than the text.
    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.f, 0.f, 0.f, 0.f });
    ImGui::BeginChild("##help_text", { 0.f, 0.f }, false,
        ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, { 0.75f, 0.75f, 0.86f, 1.f });

    ImGui::TextWrapped(
        "Each particle has a position and an orientation angle.  "
        "Every tick, it counts how many other particles lie within its "
        "visual radius, and how many of those are to its right vs left.");

    ImGui::Spacing();

    ImGui::TextWrapped(
        "The turn rate is determined by:");

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, { 0.55f, 0.88f, 0.70f, 1.f });
    ImGui::TextUnformatted("  angle += alpha + beta * N * sign(R - L)");
    ImGui::PopStyleColor();

    ImGui::Spacing();

    ImGui::TextWrapped(
        "where N is the total neighbour count, R and L are the right "
        "and left counts, and sign gives the turning direction.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "Alpha controls the base turn rate — higher values make "
        "particles spin in place and form rings.");

    ImGui::Spacing();

    ImGui::TextWrapped(
        "Beta scales how strongly density steers the particle.  "
        "Positive beta causes flocking; negative beta causes scattering.");

    ImGui::Spacing();

    ImGui::TextWrapped(
        "Gamma is the step size — how far each particle moves per tick "
        "in the direction of its angle.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "The spatial hash grid divides the world into a fixed array of "
        "cells.  Each tick, particles are bucketed into their cell so "
        "neighbour queries only scan the 9 adjacent cells rather than "
        "the entire population — bringing the cost from O(N\xc2\xb2) to O(N).");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "Colors are mapped from each particle's neighbour count through "
        "the gradient defined in the Particle tab.  The heatmap "
        "aggregates particle density onto a screen-resolution grid and "
        "is used automatically at high zoom-out levels.");

    ImGui::PopStyleColor(); // Text

    ImGui::EndChild();
}