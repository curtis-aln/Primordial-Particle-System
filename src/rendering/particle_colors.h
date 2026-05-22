#pragma once
#include <cstdint>
#include <SFML/Graphics/Color.hpp>

// ─────────────────────────────────────────────────────────────────────────────
//  ParticleColorScheme
//
//  Runtime-editable gradient used by PPS_Renderer::get_color().
//  Both the renderer and ParticleTab run on the render thread, so direct
//  reads/writes are safe — no mutex needed.
//
//  Color channels are stored in [0, 1] float range to match ImGui's
//  ColorEdit/ColorPicker API directly.
//
//  Threshold layout (4 stops, 3 boundary values):
//    [0 .. stops[1].threshold)  → lerp stop[0] → stop[1]
//    [stops[1].threshold .. stops[2].threshold) → lerp stop[1] → stop[2]
//    [stops[2].threshold .. stops[3].threshold) → lerp stop[2] → stop[3]
//    [stops[3].threshold ..)    → stop[3] solid
// ─────────────────────────────────────────────────────────────────────────────

struct ParticleColorStop
{
    float col[3];       // RGB in [0, 1]
    float threshold;    // nearby-count value at which this stop begins
    // (stop[0].threshold is always 0 and not editable)
};

struct ParticleColorScheme
{
    static constexpr int NUM_STOPS = 4;

    ParticleColorStop stops[NUM_STOPS] = {
        { { 50.f / 255.f, 1.0f,         0.0f         },  0.f  }, // Green
        { { 0.0f,       0.0f,         1.0f         }, 22.f  }, // Blue
        { { 1.0f,       192.f / 255.f,  203.f / 255.f  }, 34.f  }, // Pink
        { { 1.0f,       0.0f,         0.0f         }, 40.f  }, // Red
    };

    uint8_t global_alpha = 200;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Globals — defined in PPS_renderer.cpp
// ─────────────────────────────────────────────────────────────────────────────
extern ParticleColorScheme g_color_scheme;

// Set to true by ParticleTab when any color/threshold changes.
// Checked by PPS_Renderer::render_particles() to invalidate the color cache.
extern bool g_color_scheme_dirty;

// ─────────────────────────────────────────────────────────────────────────────
//  interpolate_scheme  — used by get_color() in PPS_renderer.cpp and by
//  ParticleTab's gradient preview (converts to sf::Color for the preview).
// ─────────────────────────────────────────────────────────────────────────────
inline sf::Color interpolate_scheme(float nearby_count, const ParticleColorScheme& s)
{
    const uint8_t a = s.global_alpha;

    auto to_sf = [&](const ParticleColorStop& stop) -> sf::Color {
        return sf::Color{
            static_cast<uint8_t>(stop.col[0] * 255.f),
            static_cast<uint8_t>(stop.col[1] * 255.f),
            static_cast<uint8_t>(stop.col[2] * 255.f),
            a
        };
        };

    auto lerp_stops = [&](const ParticleColorStop& c0,
        const ParticleColorStop& c1,
        float f) -> sf::Color {
            return sf::Color{
                static_cast<uint8_t>((c0.col[0] + f * (c1.col[0] - c0.col[0])) * 255.f),
                static_cast<uint8_t>((c0.col[1] + f * (c1.col[1] - c0.col[1])) * 255.f),
                static_cast<uint8_t>((c0.col[2] + f * (c1.col[2] - c0.col[2])) * 255.f),
                a
            };
        };

    if (nearby_count <= 0.f) return to_sf(s.stops[0]);

    for (int i = 0; i < ParticleColorScheme::NUM_STOPS - 1; ++i)
    {
        const float t0 = s.stops[i].threshold;
        const float t1 = s.stops[i + 1].threshold;
        if (nearby_count < t1)
        {
            const float span = t1 - t0;
            const float f = (span > 0.f) ? (nearby_count - t0) / span : 0.f;
            return lerp_stops(s.stops[i], s.stops[i + 1], f);
        }
    }

    return to_sf(s.stops[ParticleColorScheme::NUM_STOPS - 1]);
}