#include "particle_tab.h"
#include "../../../particle_system/particle_colors.h"
#include "../../../settings.h"

#include <imgui.h>
#include <cstdio>
#include <algorithm>

// ── Gradient preview ──────────────────────────────────────────────────────────
// Draws a smooth gradient bar directly onto the ImDrawList.

static void draw_gradient_preview(float width, float height = 14.f)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2      pos = ImGui::GetCursorScreenPos();

    // Round the preview to the last stop's threshold so it fills [0, max]
    const float max_t = g_color_scheme.stops[ParticleColorScheme::NUM_STOPS - 1].threshold;
    const int   segs = 80;

    for (int i = 0; i < segs; ++i)
    {
        const float t0 = max_t * static_cast<float>(i) / segs;
        const float t1 = max_t * static_cast<float>(i + 1) / segs;

        auto to_col32 = [&](float t) -> ImU32 {
            const sf::Color c = interpolate_scheme(t, g_color_scheme);
            return IM_COL32(c.r, c.g, c.b, 255);
            };

        const float x0 = pos.x + width * static_cast<float>(i) / segs;
        const float x1 = pos.x + width * static_cast<float>(i + 1) / segs;

        dl->AddRectFilledMultiColor(
            { x0, pos.y }, { x1, pos.y + height },
            to_col32(t0), to_col32(t1),
            to_col32(t1), to_col32(t0));
    }

    // Invisible dummy to advance the cursor past the drawn area
    ImGui::Dummy({ width, height });
}

// ── Color stop editor ─────────────────────────────────────────────────────────

static void draw_color_stop_row(int i, ParticleColorStop& stop,
    float prev_threshold, float next_max)
{
    char id_btn[32]; std::snprintf(id_btn, sizeof(id_btn), "##cb%d", i);
    char id_pick[32]; std::snprintf(id_pick, sizeof(id_pick), "##cp%d", i);
    char id_thr[32]; std::snprintf(id_thr, sizeof(id_thr), "##ct%d", i);
    char id_lbl[32]; std::snprintf(id_lbl, sizeof(id_lbl), "Stop %d", i + 1);

    // Coloured swatch button — opens colour picker popup
    ImVec4 swatch = { stop.col[0], stop.col[1], stop.col[2], 1.f };
    if (ImGui::ColorButton(id_btn, swatch,
        ImGuiColorEditFlags_NoAlpha |
        ImGuiColorEditFlags_NoBorder,
        { 20.f, 20.f }))
        ImGui::OpenPopup(id_pick);

    if (ImGui::BeginPopup(id_pick))
    {
        ImGui::TextUnformatted(id_lbl);
        ImGui::Separator();
        if (ImGui::ColorPicker3(id_pick, stop.col,
            ImGuiColorEditFlags_NoSidePreview |
            ImGuiColorEditFlags_NoSmallPreview))
            g_color_scheme_dirty = true;
        ImGui::EndPopup();
    }

    ImGui::SameLine(0.f, 6.f);

    // Label
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.62f, 0.62f, 0.78f, 1.f });
    ImGui::Text("%-6s", id_lbl);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.f, 4.f);

    // Threshold slider (first stop is always 0, not editable)
    if (i == 0)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.38f, 0.38f, 0.50f, 1.f });
        ImGui::TextUnformatted("  start");
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::SetNextItemWidth(-1.f);
        float lo = prev_threshold + 0.5f;
        float hi = next_max - 0.5f;
        if (hi < lo) hi = lo + 1.f;

        if (ImGui::SliderFloat(id_thr, &stop.threshold, lo, hi, "%.1f"))
            g_color_scheme_dirty = true;
    }
}

// ── Physics section ───────────────────────────────────────────────────────────

void ParticleTab::draw_physics(SimCtx& ctx)
{
    section_header("PHYSICS");

    // IMGUI_TODO: SetAlpha / SetBeta / SetGamma commands must be handled in
    //   Simulation::resolve_modifications():
    //     case CommandType::SetAlpha: particle_system_.alpha = cmd.float_val; break;
    //     case CommandType::SetBeta:  particle_system_.beta  = cmd.float_val; break;
    //     case CommandType::SetGamma: particle_system_.gamma = cmd.float_val; break;
    //   (gamma also requires PPS_Settings::gamma to be non-constexpr — see settings.h)

    ImGui::SetNextItemWidth(-1.f);
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Alpha  (turn rate)");
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::SliderFloat("##alpha", &m_alpha_, 0.f, 360.f, "%.1f deg"))
        ctx.push({ CommandType::SetAlpha, {}, m_alpha_ });

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Beta   (neighbour scale)");
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::SliderFloat("##beta", &m_beta_, -30.f, 30.f, "%.2f deg/n"))
        ctx.push({ CommandType::SetBeta, {}, m_beta_ });

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Gamma  (step size)");
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::SliderFloat("##gamma", &m_gamma_, 10.f, 600.f, "%.1f"))
        ctx.push({ CommandType::SetGamma, {}, m_gamma_ });
}

// ── Colour scheme section ─────────────────────────────────────────────────────

void ParticleTab::draw_color_scheme()
{
    section_header("COLOR SCHEME");

    // Global alpha
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Particle Alpha");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.f);
    int alpha_i = static_cast<int>(g_color_scheme.global_alpha);
    if (ImGui::SliderInt("##galpha", &alpha_i, 20, 255))
    {
        g_color_scheme.global_alpha = static_cast<uint8_t>(alpha_i);
        g_color_scheme_dirty = true;
    }

    thin_sep();

    // Per-stop rows
    for (int i = 0; i < ParticleColorScheme::NUM_STOPS; ++i)
    {
        const float prev_thr = (i > 0)
            ? g_color_scheme.stops[i - 1].threshold : 0.f;
        const float next_max = (i < ParticleColorScheme::NUM_STOPS - 1)
            ? g_color_scheme.stops[i + 1].threshold : 80.f;

        draw_color_stop_row(i, g_color_scheme.stops[i], prev_thr, next_max);
    }

    // Gradient preview
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.38f, 0.38f, 0.50f, 1.f });
    ImGui::TextUnformatted("Preview");
    ImGui::PopStyleColor();
    draw_gradient_preview(ImGui::GetContentRegionAvail().x);
}

// ── Entry point ───────────────────────────────────────────────────────────────

void ParticleTab::draw(const SimSnapshot& /*snap*/, SimCtx& ctx)
{
    // Sync local copies from live PPS_Settings on the very first draw
    if (m_first_draw_)
    {
        m_alpha_ = PPS_Settings::alpha;
        m_beta_ = PPS_Settings::beta;
        m_gamma_ = PPS_Settings::gamma;
        m_first_draw_ = false;
    }

    draw_physics(ctx);
    draw_color_scheme();
}