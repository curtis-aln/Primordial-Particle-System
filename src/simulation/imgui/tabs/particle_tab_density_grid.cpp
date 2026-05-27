// ─────────────────────────────────────────────────────────────────────────────
// particle_tab_density_grid.cpp
//
// Drop-in addition to particle_tab.cpp.
// Either #include this file at the bottom of particle_tab.cpp, or paste the
// two functions (draw_density_grid + the updated draw()) directly into it.
//
// Dependencies already present in particle_tab.cpp:
//   #include <imgui.h>
//   #include "../settings.h"   — PPS_Settings::visual_radius
// ─────────────────────────────────────────────────────────────────────────────

#include "particle_tab.h"

#include <imgui.h>
#include <cstdio>
#include "../../../settings.h"   // PPS_Settings


// ── Internal helpers (already exist in particle_tab.cpp — remove if merging) ─

// section_header and thin_sep are defined in the existing particle_tab.cpp.
// Declarations are in particle_tab.h; no re-definition needed here.


// ─────────────────────────────────────────────────────────────────────────────
// draw_density_grid()
// ─────────────────────────────────────────────────────────────────────────────

void ParticleTab::draw_density_grid(SimCtx& ctx)
{
    section_header("DENSITY GRID");

    const float vr = PPS_Settings::visual_radius;

    // ── Cell size presets ─────────────────────────────────────────────────────
    // Each entry: { label, multiplier of visual_radius }
    static constexpr struct { const char* label; float mul; } k_presets[] = {
        { "vr/4  (high accuracy)",  0.25f },
        { "vr/2  (default)",        0.50f },
        { "vr    (coarse)",         1.00f },
        { "vr×2  (very coarse)",    2.00f },
    };
    static constexpr int k_num_presets = static_cast<int>(
        sizeof(k_presets) / sizeof(k_presets[0]));

    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Cell Size");
    ImGui::PopStyleColor();

    bool preset_changed = false;
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::BeginCombo("##dg_cell_size",
        k_presets[m_dg_.cell_preset].label))
    {
        for (int i = 0; i < k_num_presets; ++i)
        {
            const bool selected = (m_dg_.cell_preset == i);
            if (ImGui::Selectable(k_presets[i].label, selected))
            {
                m_dg_.cell_preset = i;
                m_dg_.rebuild_pending = true;
                preset_changed = true;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Show derived cell size value
    if (preset_changed || m_dg_.cell_preset >= 0)
    {
        const float cell_sz = vr * k_presets[m_dg_.cell_preset].mul;
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.38f, 0.38f, 0.50f, 1.f });
        ImGui::Text("  → %.1f world units / cell", static_cast<double>(cell_sz));
        ImGui::PopStyleColor();
    }

    if (m_dg_.rebuild_pending)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.85f, 0.65f, 0.20f, 1.f });
        ImGui::TextUnformatted("  ⚠  Rebuild pending…");
        ImGui::PopStyleColor();
    }

    // Rebuild button
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::Button("Rebuild Grid##dg_rebuild", { -1.f, 0.f }))
    {
        m_dg_.rebuild_pending = true;
        // IMGUI_TODO: push a CommandType::RebuildDensityGrid command so the
        // sim thread re-constructs DensityGrid with the new cell size:
        //
        //   const float new_cell_sz = vr * k_presets[m_dg_.cell_preset].mul;
        //   ctx.push({ CommandType::RebuildDensityGrid, {}, new_cell_sz });
        //
        // After the sim rebuilds, call ParticleTab::notify_grid_rebuilt(w, h).
    }

    // Read-only grid dimensions (updated by notify_grid_rebuilt())
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.38f, 0.38f, 0.50f, 1.f });
    ImGui::Text("  Grid: %s", m_dg_dim_str_);
    ImGui::PopStyleColor();

    thin_sep();

    // ── Smoothing mode ────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Smoothing");
    ImGui::PopStyleColor();

    {
        bool gauss = m_dg_.gaussian_mode;
        if (ImGui::RadioButton("Gaussian##dg_gauss", gauss))
        {
            m_dg_.gaussian_mode = true;
            // IMGUI_TODO: push CommandType::SetDensityGaussianMode with value 1.f
        }
        ImGui::SameLine(0.f, 12.f);
        if (ImGui::RadioButton("Box (faster)##dg_box", !gauss))
        {
            m_dg_.gaussian_mode = false;
            // IMGUI_TODO: push CommandType::SetDensityGaussianMode with value 0.f
        }
    }

    if (m_dg_.gaussian_mode)
    {
        // Gaussian sigma multiplier: sigma = visual_radius * inv_s * sigma_mul
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
        ImGui::TextUnformatted("  Sigma multiplier");
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(-1.f);
        if (ImGui::SliderFloat("##dg_sigma", &m_dg_.sigma_mul, 0.2f, 1.0f, "%.2f"))
        {
            // IMGUI_TODO: push CommandType::SetDensitySigmaMul with value m_dg_.sigma_mul
            // In DensityGrid::smooth_gaussian(), replace the hardcoded 0.5f:
            //   const float sigma = PPS_Settings::visual_radius * m_inv_s * sigma_mul;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.30f, 0.30f, 0.42f, 1.f });
        const float sigma_world = vr * m_dg_.sigma_mul;
        ImGui::Text("  → σ ≈ %.1f world units", static_cast<double>(sigma_world));
        ImGui::PopStyleColor();
    }
    else
    {
        // Box filter cascade passes (1–5)
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
        ImGui::TextUnformatted("  Cascade passes");
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(-1.f);
        if (ImGui::SliderInt("##dg_box_passes", &m_dg_.box_passes, 1, 5))
        {
            // IMGUI_TODO: push CommandType::SetDensityBoxPasses with value (float)m_dg_.box_passes
            // In DensityGrid::smooth_box(), replace the hardcoded `3` in the pass loop:
            //   for (int pass = 0; pass < box_passes; ++pass)
            // and update the norm: 1 / (kernel_area ^ box_passes)
        }

        // Quality indicator
        static constexpr const char* k_pass_desc[] = {
            "rough", "ok", "good (default)", "very good", "Gaussian-like"
        };
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.30f, 0.30f, 0.42f, 1.f });
        ImGui::Text("  → %s", k_pass_desc[m_dg_.box_passes - 1]);
        ImGui::PopStyleColor();
    }

    thin_sep();

    // ── Sampling mode ─────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Particle sampler");
    ImGui::PopStyleColor();

    static constexpr const char* k_sampler_labels[] = {
        "Bilinear  (fastest)",
        "Bicubic   (default, smooth)",
        "IDW-8     (alternative)",
    };

    ImGui::SetNextItemWidth(-1.f);
    const int sampler_idx = static_cast<int>(m_dg_.sampler);
    int new_sampler = sampler_idx;
    if (ImGui::BeginCombo("##dg_sampler", k_sampler_labels[sampler_idx]))
    {
        for (int i = 0; i < 3; ++i)
        {
            const bool sel = (sampler_idx == i);
            if (ImGui::Selectable(k_sampler_labels[i], sel))
                new_sampler = i;
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (new_sampler != sampler_idx)
    {
        m_dg_.sampler = static_cast<DensitySampler>(new_sampler);
        // IMGUI_TODO: push CommandType::SetDensitySampler with value (float)new_sampler
        // In DensityGrid::update_particle_density(), swap the hardcoded bicubic calls:
        //
        //   switch (sampler) {
        //     case DensitySampler::Bilinear: N = bilerp(m_D, x, y); ... break;
        //     case DensitySampler::Bicubic:  N = bicubic(m_D, x, y); ... break;
        //     case DensitySampler::Sample8:  N = sample8(m_D, x, y); ... break;
        //   }
    }

    thin_sep();

    // ── Debug overlay ─────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
    ImGui::TextUnformatted("Debug overlay");
    ImGui::PopStyleColor();

    // Radio buttons for channel selection
    static constexpr struct { const char* label; DensityOverlayChannel ch; } k_channels[] = {
        { "None",            DensityOverlayChannel::None    },
        { "Density",         DensityOverlayChannel::Density },
        { "Gradient X",      DensityOverlayChannel::GradX   },
        { "Gradient Y",      DensityOverlayChannel::GradY   },
        { "Grad magnitude",  DensityOverlayChannel::GradMag },
    };

    for (int i = 0; i < 5; ++i)
    {
        const bool active = (m_dg_.overlay_channel == k_channels[i].ch);
        if (ImGui::RadioButton(k_channels[i].label, active))
            m_dg_.overlay_channel = k_channels[i].ch;
        if (i < 4) ImGui::SameLine(0.f, 16.f);
    }

    // Overlay controls — only shown when overlay is active
    if (m_dg_.overlay_channel != DensityOverlayChannel::None)
    {
        ImGui::Spacing();

        // Overlay opacity
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
        ImGui::TextUnformatted("  Opacity");
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(-1.f);
        int alpha_i = static_cast<int>(m_dg_.overlay_alpha);
        if (ImGui::SliderInt("##dg_ov_alpha", &alpha_i, 10, 255))
            m_dg_.overlay_alpha = static_cast<uint8_t>(alpha_i);

        // Peak scale
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.52f, 0.52f, 0.66f, 1.f });
        ImGui::TextUnformatted("  Peak scale");
        ImGui::PopStyleColor();
        ImGui::SameLine(0.f, 8.f);
        if (ImGui::Checkbox("Auto##dg_auto", &m_dg_.overlay_auto_peak))
        {
            // nothing extra needed — renderer reads overlay_auto_peak directly
        }
        if (!m_dg_.overlay_auto_peak)
        {
            ImGui::SetNextItemWidth(-1.f);
            ImGui::SliderFloat("##dg_peak", &m_dg_.overlay_peak, 1.f, 2000.f,
                "%.0f", ImGuiSliderFlags_Logarithmic);
        }

        // Channel-specific legend hint
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.30f, 0.30f, 0.42f, 1.f });
        switch (m_dg_.overlay_channel)
        {
        case DensityOverlayChannel::Density:
        case DensityOverlayChannel::GradMag:
            ImGui::TextUnformatted("  Colourmap: black → navy → cyan → yellow → white");
            break;
        case DensityOverlayChannel::GradX:
            ImGui::TextUnformatted("  Colourmap: blue (−) → black (0) → red (+)  [X]");
            break;
        case DensityOverlayChannel::GradY:
            ImGui::TextUnformatted("  Colourmap: blue (−) → black (0) → red (+)  [Y]");
            break;
        default: break;
        }
        ImGui::PopStyleColor();

        // IMGUI_TODO: wire overlay into PPS_Renderer::render():
        //
        //   const auto& dgs = particle_tab.density_grid_settings();
        //   if (dgs.overlay_channel != DensityOverlayChannel::None)
        //   {
        //       const float peak = dgs.overlay_auto_peak ? 0.f : dgs.overlay_peak;
        //       m_dg_overlay.upload(density_grid, dgs.overlay_channel, peak);
        //       m_dg_overlay.draw(*window_, dgs.overlay_alpha);
        //   }
        //
        // DensityGrid& density_grid must be accessible from PPS_Renderer — the
        // cleanest approach is to add a const DensityGrid* pointer to SimSnapshot:
        //
        //   struct SimSnapshot {
        //       ...
        //       const DensityGrid* density_grid = nullptr;  // IMGUI_TODO
        //   };
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ParticleTab::draw() — add the new section at the bottom
// Replace (or augment) the existing draw() in particle_tab.cpp with this.
// ─────────────────────────────────────────────────────────────────────────────

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
    draw_density_grid(ctx);
}