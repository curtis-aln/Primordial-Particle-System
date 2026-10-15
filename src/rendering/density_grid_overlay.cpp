#include "density_grid_overlay.h"
#include "../particle_system/density_grid.h"

#include <algorithm>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// Colour helpers
// ─────────────────────────────────────────────────────────────────────────────

sf::Color DensityGridOverlay::heat_color(float t)
{
    // Five-stop sequential ramp: black → navy → cyan → yellow → white
    struct Stop { float t; uint8_t r, g, b; };
    static constexpr Stop stops[] = {
        { 0.00f,   0,   0,   0 },
        { 0.25f,   0,   0, 200 },
        { 0.50f,   0, 210, 255 },
        { 0.75f, 255, 230,   0 },
        { 1.00f, 255, 255, 255 },
    };

    t = std::clamp(t, 0.f, 1.f);

    for (int i = 1; i < 5; ++i)
    {
        if (t <= stops[i].t)
        {
            const float s = (t - stops[i - 1].t) / (stops[i].t - stops[i - 1].t);
            auto lerp = [](uint8_t a, uint8_t b, float f) {
                return static_cast<uint8_t>(static_cast<float>(a)
                    + (static_cast<float>(b) - static_cast<float>(a)) * f);
                };
            return {
                lerp(stops[i - 1].r, stops[i].r, s),
                lerp(stops[i - 1].g, stops[i].g, s),
                lerp(stops[i - 1].b, stops[i].b, s),
                255u
            };
        }
    }
    return { 255, 255, 255, 255 };
}

sf::Color DensityGridOverlay::div_color(float t)
{
    // Diverging: blue → near-black → red
    // t in [0, 1];  t = 0.5 maps to zero (black).
    t = std::clamp(t, 0.f, 1.f);

    if (t < 0.5f)
    {
        // Negative side: full blue at 0, fade to black at 0.5
        const float s = 1.f - t * 2.f;
        return { 0u, 0u, static_cast<uint8_t>(220.f * s), 255u };
    }
    else
    {
        // Positive side: black at 0.5, full red at 1
        const float s = (t - 0.5f) * 2.f;
        return { static_cast<uint8_t>(220.f * s), 0u, 0u, 255u };
    }
}

// Precompute both LUTs at construction time — called once, zero per-frame cost.
void DensityGridOverlay::build_luts()
{
    for (int i = 0; i < LUT_SIZE; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(LUT_SIZE - 1);
        m_lut_heat[i] = pack_rgba(heat_color(t));
        m_lut_div[i] = pack_rgba(div_color(t));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// resize()
// ─────────────────────────────────────────────────────────────────────────────

void DensityGridOverlay::resize(unsigned gw, unsigned gh,
    unsigned sw, unsigned sh)
{
    m_gw = gw; m_gh = gh;
    m_sw = sw; m_sh = sh;
    m_valid = false;
    m_smoothed_peak = 1.f;

    // Allocate pixel scratch once; cleared in upload() per channel.
    m_pixels.assign(static_cast<size_t>(gw) * gh * 4u, 0u);

    if (!m_texture.resize({ gw, gh }))
        return;   // SFML allocation failure — draw() will be a no-op

    m_sprite = sf::Sprite(m_texture);
    m_valid = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// upload()
// ─────────────────────────────────────────────────────────────────────────────

void DensityGridOverlay::upload(const DensityGrid& grid,
    DensityOverlayChannel channel,
    float                 peak_override)
{
    if (channel == DensityOverlayChannel::None || !m_valid)
        return;

    const unsigned n = m_gw * m_gh;

    // ── 1. Compute raw peak ───────────────────────────────────────────────────
    float raw_peak = 0.f;

    if (peak_override > 0.f)
    {
        raw_peak = peak_override;
    }
    else
    {
        // Single pass over the relevant data.
        // For GradMag: compute magnitude inline to avoid a separate buffer.
        switch (channel)
        {
        case DensityOverlayChannel::Density:
        {
            const float* d = grid.density().data();
            for (unsigned i = 0; i < n; ++i)
                if (d[i] > raw_peak) raw_peak = d[i];
            break;
        }
        case DensityOverlayChannel::GradX:
        {
            const float* gx = grid.gradient_x().data();
            for (unsigned i = 0; i < n; ++i)
            {
                const float v = std::abs(gx[i]);
                if (v > raw_peak) raw_peak = v;
            }
            break;
        }
        case DensityOverlayChannel::GradY:
        {
            const float* gy = grid.gradient_y().data();
            for (unsigned i = 0; i < n; ++i)
            {
                const float v = std::abs(gy[i]);
                if (v > raw_peak) raw_peak = v;
            }
            break;
        }
        case DensityOverlayChannel::GradMag:
        {
            const float* gx = grid.gradient_x().data();
            const float* gy = grid.gradient_y().data();
            for (unsigned i = 0; i < n; ++i)
            {
                const float mag = std::sqrt(gx[i] * gx[i] + gy[i] * gy[i]);
                if (mag > raw_peak) raw_peak = mag;
            }
            break;
        }
        default: break;
        }
    }

    if (raw_peak < 1e-9f) raw_peak = 1.f;

    // EMA smoothing prevents flickering when density spikes briefly.
    m_smoothed_peak = m_smoothed_peak * k_peak_ema + raw_peak * (1.f - k_peak_ema);
    const float inv_peak = 1.f / m_smoothed_peak;

    // Convenience: write directly as packed uint32 into the byte buffer.
    uint32_t* dst = reinterpret_cast<uint32_t*>(m_pixels.data());

    // ── 2. Tone-map: single tight loop per channel ────────────────────────────
    const float lut_scale = static_cast<float>(LUT_SIZE - 1);

    switch (channel)
    {
    case DensityOverlayChannel::Density:
    {
        const float* src = grid.density().data();
        for (unsigned i = 0; i < n; ++i)
        {
            const int idx = static_cast<int>(
                std::clamp(src[i] * inv_peak, 0.f, 1.f) * lut_scale);
            dst[i] = m_lut_heat[idx];
        }
        break;
    }
    case DensityOverlayChannel::GradX:
    {
        const float* src = grid.gradient_x().data();
        const float  scale = 0.5f * inv_peak;
        for (unsigned i = 0; i < n; ++i)
        {
            // Map [-peak, +peak] → [0, 1]
            const int idx = static_cast<int>(
                std::clamp(src[i] * scale + 0.5f, 0.f, 1.f) * lut_scale);
            dst[i] = m_lut_div[idx];
        }
        break;
    }
    case DensityOverlayChannel::GradY:
    {
        const float* src = grid.gradient_y().data();
        const float  scale = 0.5f * inv_peak;
        for (unsigned i = 0; i < n; ++i)
        {
            const int idx = static_cast<int>(
                std::clamp(src[i] * scale + 0.5f, 0.f, 1.f) * lut_scale);
            dst[i] = m_lut_div[idx];
        }
        break;
    }
    case DensityOverlayChannel::GradMag:
    {
        const float* gx = grid.gradient_x().data();
        const float* gy = grid.gradient_y().data();
        for (unsigned i = 0; i < n; ++i)
        {
            const float mag = std::sqrt(gx[i] * gx[i] + gy[i] * gy[i]);
            const int idx = static_cast<int>(
                std::clamp(mag * inv_peak, 0.f, 1.f) * lut_scale);
            dst[i] = m_lut_heat[idx];
        }
        break;
    }
    default: break;
    }

    m_texture.update(m_pixels.data());
}

// ─────────────────────────────────────────────────────────────────────────────
// draw()
// ─────────────────────────────────────────────────────────────────────────────

void DensityGridOverlay::draw(sf::RenderWindow& window, uint8_t alpha)
{
    if (!m_valid) return;

    const sf::View saved = window.getView();
    window.setView(window.getDefaultView());

    // Scale the tiny grid texture up to fill the full screen.
    m_sprite.setScale({
        static_cast<float>(m_sw) / static_cast<float>(m_gw),
        static_cast<float>(m_sh) / static_cast<float>(m_gh)
        });
    m_sprite.setPosition({ 0.f, 0.f });
    m_sprite.setColor(sf::Color(255, 255, 255, alpha));

    // Use additive blend so the overlay brightens rather than obscures particles.
    sf::RenderStates rs;
    rs.blendMode = sf::BlendAdd;
    window.draw(m_sprite, rs);

    window.setView(saved);
}