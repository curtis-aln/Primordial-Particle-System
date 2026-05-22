// density_heatmap.h
#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "../settings.h"

class DensityHeatmap
{
public:
    float m_screen_w = static_cast<float>(SimulationSettings::screen_width);
    float m_screen_h = static_cast<float>(SimulationSettings::screen_height);

    // ── Construction ──────────────────────────────────────────────────────────

    DensityHeatmap(float world_w, float world_h,
        unsigned int screen_w, unsigned int screen_h,
        unsigned int downsample = 2);


    void set_trail_decay(float decay);

    void clear();

    void scatter(const float* px, const float* py, int n, const sf::View& view);

    void upload(uint32_t fixed_peak = 0u);

    // alpha: 0 = invisible, 255 = fully opaque
    void draw(sf::RenderWindow& window, uint8_t alpha = 255);

    // ── Tunables ──────────────────────────────────────────────────────────────

    struct GradientStop { float t; sf::Color colour; };

    void set_gradient(std::vector<GradientStop> stops);

private:
    // ── Gradient LUT ──────────────────────────────────────────────────────────

    static constexpr int LUT_SIZE = 512;

    void precompute_lut();

    sf::Color interpolate_gradient(float t) const;

    sf::Color sample_lut(float t) const;

    static uint8_t lerp_u8(uint8_t a, uint8_t b, float t);

    // ── Members ───────────────────────────────────────────────────────────────
    float m_smoothed_peak = 1.f;
    float m_peak_ema = 0.96f; // 0 = no smoothing, closer to 1 = more smoothing

    float        m_world_w, m_world_h;
    unsigned int m_tex_w, m_tex_h;
    float        m_inv_world_x, m_inv_world_y;
    float        m_trail_decay = 0.f;  // 0 = disabled

    std::vector<float>    m_counts;    // float so decay works smoothly
    std::vector<uint8_t>  m_pixels;
    sf::Texture           m_texture;
    sf::Sprite            m_sprite;

    std::array<sf::Color, LUT_SIZE> m_lut;
    std::vector<GradientStop>       m_stops;
};