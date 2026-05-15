// density_heatmap.h
#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "../settings.h"

// ─────────────────────────────────────────────────────────────────────────────
//  DensityHeatmap
//
//  Accumulates particle positions into a downsampled count grid, maps density
//  to colour via a precomputed gradient LUT, then uploads to a GPU texture and
//  draws it stretched to the target window.
//
//  Usage per frame:
//      heatmap.clear();
//      heatmap.scatter(positions_x, positions_y, n);
//      heatmap.upload();
//      heatmap.draw(window);
// ─────────────────────────────────────────────────────────────────────────────
class DensityHeatmap
{
public:
    float m_screen_w = SimulationSettings::screen_width;
    float m_screen_h = SimulationSettings::screen_height;

    // ── Construction ──────────────────────────────────────────────────────────

    // In the constructor, replace the sprite setup:
    DensityHeatmap(float world_w, float world_h,
        unsigned int screen_w, unsigned int screen_h,
        unsigned int downsample = 2)
        : m_world_w(world_w)
        , m_world_h(world_h)
        , m_tex_w(screen_w / downsample)
        , m_tex_h(screen_h / downsample)
        , m_inv_world_x(static_cast<float>(m_tex_w) / world_w)
        , m_inv_world_y(static_cast<float>(m_tex_h) / world_h)
        , m_screen_w(screen_w)
        , m_screen_h(screen_h)
		, m_sprite(m_texture)
    {
        const size_t n = static_cast<size_t>(m_tex_w) * m_tex_h;
        m_counts.resize(n, 0u);
        m_pixels.resize(n * 4, 0u);

        if (!m_texture.resize({ m_tex_w, m_tex_h }))
            std::cout << "[DensityHeatmap] Failed to create texture\n";

        precompute_lut();
    }

    // ── Per-frame API ─────────────────────────────────────────────────────────

    void clear()
    {
        std::fill(m_counts.begin(), m_counts.end(), 0u);
    }

    void scatter(const std::vector<float>& px, const std::vector<float>& py,
        int n, const sf::View& view)
    {
        // View transform: world → normalised screen → texture pixel
        const sf::Vector2f view_center = view.getCenter();
        const sf::Vector2f view_size = view.getSize();

        const float inv_vw = 1.f / view_size.x;
        const float inv_vh = 1.f / view_size.y;

        for (int i = 0; i < n; ++i)
        {
            // Normalised device coords [-0.5, 0.5]
            const float nx = (px[i] - view_center.x) * inv_vw + 0.5f;
            const float ny = (py[i] - view_center.y) * inv_vh + 0.5f;

            // Texture pixel coords
            const int tx = static_cast<int>(nx * static_cast<float>(m_tex_w));
            const int ty = static_cast<int>(ny * static_cast<float>(m_tex_h));

            if (tx >= 0 && tx < static_cast<int>(m_tex_w) &&
                ty >= 0 && ty < static_cast<int>(m_tex_h))
            {
                ++m_counts[ty * m_tex_w + tx];
            }
        }
    }

    // Call after scatter(). Finds peak density, maps counts → LUT → pixel buffer.
    void upload(uint32_t fixed_peak = 0u)
    {
        const uint32_t peak = fixed_peak > 0u
            ? fixed_peak
            : *std::max_element(m_counts.begin(), m_counts.end());

        if (peak == 0u)
            return;

        const float inv_peak = 1.f / static_cast<float>(peak);
        const size_t n = m_counts.size();

        for (size_t i = 0; i < n; ++i)
        {
            const float t = std::clamp(static_cast<float>(m_counts[i]) * inv_peak, 0.f, 1.f);
            const sf::Color colour = sample_lut(t);
            const size_t   base = i * 4;

            m_pixels[base + 0] = colour.r;
            m_pixels[base + 1] = colour.g;
            m_pixels[base + 2] = colour.b;
            m_pixels[base + 3] = colour.a;
        }

        m_texture.update(m_pixels.data());
    }

    // Replace draw() entirely — reset view so camera doesn't affect it
    void draw(sf::RenderWindow& window)
    {
        const sf::View saved_view = window.getView();

        // Draw in raw screen space, no camera
        window.setView(window.getDefaultView());

        sf::Sprite sprite(m_texture);
        sprite.setScale({
            static_cast<float>(SimulationSettings::screen_width) / static_cast<float>(m_tex_w),
            static_cast<float>(SimulationSettings::screen_height) / static_cast<float>(m_tex_h)
            });
        window.draw(sprite);

        window.setView(saved_view);
    }

    // ── Tunables ──────────────────────────────────────────────────────────────

    // Replace to taste. Each stop is { t ∈ [0,1], RGBA }.
    // Default: black → deep blue → cyan → green → yellow → white
    struct GradientStop { float t; sf::Color colour; };

    void set_gradient(std::vector<GradientStop> stops)
    {
        m_stops = std::move(stops);
        precompute_lut();
    }

private:
    // ── Gradient LUT ──────────────────────────────────────────────────────────

    static constexpr int LUT_SIZE = 512;

    void precompute_lut()
    {
        if (m_stops.empty())
        {
            m_stops = {
                { 0.00f, { 0,   0,   0,   255 } },
                { 0.15f, { 0,   0,   180, 255 } },
                { 0.35f, { 0,   200, 255, 255 } },
                { 0.55f, { 0,   255, 80,  255 } },
                { 0.75f, { 255, 240, 0,   255 } },
                { 1.00f, { 255, 255, 255, 255 } },
            };
        }

        for (int i = 0; i < LUT_SIZE; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(LUT_SIZE - 1);
            m_lut[i] = interpolate_gradient(t);
        }
    }

    sf::Color interpolate_gradient(float t) const
    {
        if (m_stops.empty()) return sf::Color::Black;
        if (t <= m_stops.front().t) return m_stops.front().colour;
        if (t >= m_stops.back().t)  return m_stops.back().colour;

        for (size_t i = 1; i < m_stops.size(); ++i)
        {
            if (t <= m_stops[i].t)
            {
                const float lo = m_stops[i - 1].t;
                const float hi = m_stops[i].t;
                const float s = (t - lo) / (hi - lo);

                const sf::Color& a = m_stops[i - 1].colour;
                const sf::Color& b = m_stops[i].colour;

                return {
                    lerp_u8(a.r, b.r, s),
                    lerp_u8(a.g, b.g, s),
                    lerp_u8(a.b, b.b, s),
                    lerp_u8(a.a, b.a, s),
                };
            }
        }
        return m_stops.back().colour;
    }

    sf::Color sample_lut(float t) const
    {
        const int idx = static_cast<int>(t * static_cast<float>(LUT_SIZE - 1));
        return m_lut[std::clamp(idx, 0, LUT_SIZE - 1)];
    }

    static uint8_t lerp_u8(uint8_t a, uint8_t b, float t)
    {
        return static_cast<uint8_t>(static_cast<float>(a) + t * static_cast<float>(b - a));
    }

    // ── Members ───────────────────────────────────────────────────────────────

    float        m_world_w, m_world_h;
    unsigned int m_tex_w, m_tex_h;
    float        m_inv_world_x, m_inv_world_y;

    std::vector<uint32_t>    m_counts;   // density accumulator
    std::vector<uint8_t>     m_pixels;   // RGBA upload buffer
    sf::Texture              m_texture;
    sf::Sprite               m_sprite;

    std::array<sf::Color, LUT_SIZE> m_lut;
    std::vector<GradientStop>       m_stops;
};