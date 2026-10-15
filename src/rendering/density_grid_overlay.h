#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// DensityGridOverlay
//
// Converts any DensityGrid channel (D, Gx, Gy, |∇D|) into an SFML texture
// and draws it as a full-screen overlay on top of the simulation.
//
// DESIGN GOALS:
//   • O(W·H) per frame — single flat loop, no dynamic allocation after resize().
//   • Two precomputed 256-entry LUTs (heat + diverging) — branch-free per pixel.
//   • Handles arbitrarily large grids; texture is exactly grid_w × grid_h pixels
//     and is scaled up to screen size by the sprite transform.
//   • EMA-smoothed peak so auto-range doesn't flicker.
//
// INTEGRATION — in PPS_Renderer:
//
//   // (1) Construction:
//   DensityGridOverlay m_dg_overlay;
//
//   // (2) When DensityGrid is (re)built with a new cell size:
//   m_dg_overlay.resize(density_grid.width(),  density_grid.height(),
//                       SimulationSettings::screen_width,
//                       SimulationSettings::screen_height);
//
//   // (3) Each render frame, before drawing particles:
//   if (dg_channel != DensityOverlayChannel::None)
//   {
//       m_dg_overlay.upload(density_grid, dg_channel, dg_peak_override);
//       m_dg_overlay.draw(*window_, overlay_alpha);
//   }
// ─────────────────────────────────────────────────────────────────────────────

#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include <cstdint>

// Forward-declare to avoid pulling in the full simulation header here.
class DensityGrid;

// ── Channel selector ──────────────────────────────────────────────────────────
enum class DensityOverlayChannel : int
{
    None = 0,   // overlay disabled
    Density = 1,   // raw density field D
    GradX = 2,   // ∂D/∂x  (signed, diverging colourmap)
    GradY = 3,   // ∂D/∂y  (signed, diverging colourmap)
    GradMag = 4,   // √(Gx²+Gy²)  (unsigned, heat colourmap)
};

// ── Sampler selector (mirrors DensityGrid's three samplers) ───────────────────
enum class DensitySampler : int
{
    Bilinear = 0,  // bilerp  — fastest
    Bicubic = 1,  // bicubic — default, smoother
    Sample8 = 2,  // IDW 8-neighbour — alternative
};

class DensityGridOverlay
{
public:
    DensityGridOverlay() { build_luts(); }

    // ── Sizing ────────────────────────────────────────────────────────────────
    // Call once at startup and again whenever the DensityGrid is re-created
    // (e.g. after a cell-size change).
    void resize(unsigned grid_w, unsigned grid_h,
        unsigned screen_w, unsigned screen_h);

    // ── Per-frame update ──────────────────────────────────────────────────────
    // Reads `channel` from `grid`, tone-maps, and uploads to GPU.
    // No-op when channel == None or resize() hasn't been called yet.
    //
    // peak_override  0 → auto-range from data (EMA-smoothed)
    //               >0 → fixed scale (useful for comparing frames)
    void upload(const DensityGrid& grid,
        DensityOverlayChannel channel,
        float                 peak_override = 0.f);

    // ── Draw ──────────────────────────────────────────────────────────────────
    void draw(sf::RenderWindow& window, uint8_t alpha = 180);

    // ── Accessors ─────────────────────────────────────────────────────────────
    bool     is_valid()  const { return m_valid; }
    unsigned grid_w()    const { return m_gw; }
    unsigned grid_h()    const { return m_gh; }
    float    last_peak() const { return m_smoothed_peak; }

    // Reset the EMA peak (call after a cell-size rebuild so the new scale
    // converges quickly instead of inheriting the old one).
    void reset_peak() { m_smoothed_peak = 1.f; }

private:
    // ── Colour helpers ────────────────────────────────────────────────────────
    static constexpr int LUT_SIZE = 256;

    // Sequential: black → navy → cyan → yellow → white
    static sf::Color heat_color(float t);

    // Diverging: blue → black → red  (t=0.5 is zero)
    static sf::Color div_color(float t);

    void build_luts();

    // Pack RGBA into uint32 in the order SFML expects (R at lowest address).
    static uint32_t pack_rgba(const sf::Color& c)
    {
        return static_cast<uint32_t>(c.r)
            | static_cast<uint32_t>(c.g) << 8
            | static_cast<uint32_t>(c.b) << 16
            | static_cast<uint32_t>(c.a) << 24;
    }

    // ── Members ───────────────────────────────────────────────────────────────
    sf::Texture          m_texture;
    sf::Sprite           m_sprite;
    std::vector<uint8_t> m_pixels;   // RGBA, size = gw * gh * 4

    unsigned m_gw = 0, m_gh = 0;
    unsigned m_sw = 0, m_sh = 0;
    bool     m_valid = false;

    float m_smoothed_peak = 1.f;
    static constexpr float k_peak_ema = 0.95f;  // closer to 1 = smoother

    std::array<uint32_t, LUT_SIZE> m_lut_heat{};  // for Density / GradMag
    std::array<uint32_t, LUT_SIZE> m_lut_div{};   // for GradX / GradY
};