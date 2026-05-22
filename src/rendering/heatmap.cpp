#include "heatmap.h"

DensityHeatmap::DensityHeatmap(float world_w, float world_h,
    unsigned int screen_w, unsigned int screen_h,
    unsigned int downsample)
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
    m_counts.resize(n, 0.f);       // now float for decay blending
    m_pixels.resize(n * 4, 0u);

    if (!m_texture.resize({ m_tex_w, m_tex_h }))
        std::cout << "[DensityHeatmap] Failed to create texture\n";

    m_sprite = sf::Sprite(m_texture);

    precompute_lut();
}

// ── Trail control ─────────────────────────────────────────────────────────

// decay ∈ [0, 1]:  0 = no trail (each frame independent)
//                  0.8 = moderate trail
//                  0.95 = long trail
void DensityHeatmap::set_trail_decay(float decay)
{
    m_trail_decay = std::clamp(decay, 0.f, 1.f);
}

// ── Per-frame API ─────────────────────────────────────────────────────────

// clear() now fades rather than resets when trails are enabled.
// Call once per frame before scatter().
void DensityHeatmap::clear()
{
    if (m_trail_decay == 0.f)
        std::fill(m_counts.begin(), m_counts.end(), 0.f);
    else
        for (float& v : m_counts) v *= m_trail_decay;
}

void DensityHeatmap::scatter(const float* px, const float* py, int n, const sf::View& view)
{
    const sf::Vector2f view_center = view.getCenter();
    const sf::Vector2f view_size = view.getSize();
    const float inv_vw = 1.f / view_size.x;
    const float inv_vh = 1.f / view_size.y;

    const int W = static_cast<int>(m_tex_w);
    const int H = static_cast<int>(m_tex_h);

    for (int i = 0; i < n; ++i)
    {
        // Continuous texture-space position
        const float fx = ((px[i] - view_center.x) * inv_vw + 0.5f) * W - 0.5f;
        const float fy = ((py[i] - view_center.y) * inv_vh + 0.5f) * H - 0.5f;

        const int x0 = static_cast<int>(std::floor(fx));
        const int y0 = static_cast<int>(std::floor(fy));
        const int x1 = x0 + 1;
        const int y1 = y0 + 1;

        // Bilinear weights
        const float sx = fx - x0;
        const float sy = fy - y0;

        // Splat to 4 neighbours (bounds checked)
        auto splat = [&](int x, int y, float w) {
            if (x >= 0 && x < W && y >= 0 && y < H)
                m_counts[y * W + x] += w;
            };

        splat(x0, y0, (1.f - sx) * (1.f - sy));
        splat(x1, y0, sx * (1.f - sy));
        splat(x0, y1, (1.f - sx) * sy);
        splat(x1, y1, sx * sy);
    }
}

void DensityHeatmap::upload(uint32_t fixed_peak)
{
    float raw_peak = fixed_peak > 0u
        ? static_cast<float>(fixed_peak)
        : *std::max_element(m_counts.begin(), m_counts.end());

    if (raw_peak == 0.f) return;

    // Smooth the peak over time — kills brightness flicker
    m_smoothed_peak = m_smoothed_peak * m_peak_ema + raw_peak * (1.f - m_peak_ema);

    const float inv_peak = 1.f / m_smoothed_peak;

    const size_t n = m_counts.size();

    for (size_t i = 0; i < n; ++i)
    {
        const float t = std::clamp(m_counts[i] * inv_peak, 0.f, 1.f);
        const sf::Color colour = sample_lut(t);
        const size_t base = i * 4;

        m_pixels[base + 0] = colour.r;
        m_pixels[base + 1] = colour.g;
        m_pixels[base + 2] = colour.b;
        // Per-pixel alpha: zero-density cells are fully transparent so the
        // background shows through even when the sprite alpha is 255.
        m_pixels[base + 3] = (t > 0.f) ? colour.a : 0u;
    }

    m_texture.update(m_pixels.data());
}

// alpha: 0 = invisible, 255 = fully opaque
void DensityHeatmap::draw(sf::RenderWindow& window, uint8_t alpha)
{
    const sf::View saved_view = window.getView();
    window.setView(window.getDefaultView());

    m_sprite.setScale({
        static_cast<float>(SimulationSettings::screen_width) / static_cast<float>(m_tex_w),
        static_cast<float>(SimulationSettings::screen_height) / static_cast<float>(m_tex_h)
        });

    // Tinting with {255,255,255,alpha} modulates the whole sprite's opacity
    // without touching the RGB colours from the LUT.
    m_sprite.setColor(sf::Color(255, 255, 255, alpha));

    window.draw(m_sprite);

    window.setView(saved_view);
}

// ── Tunables ──────────────────────────────────────────────────────────────
void DensityHeatmap::set_gradient(std::vector<GradientStop> stops)
{
    m_stops = std::move(stops);
    precompute_lut();
}

// ── Gradient LUT ──────────────────────────────────────────────────────────

void DensityHeatmap::precompute_lut()
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

sf::Color DensityHeatmap::interpolate_gradient(float t) const
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

sf::Color DensityHeatmap::sample_lut(float t) const
{
    const int idx = static_cast<int>(t * static_cast<float>(LUT_SIZE - 1));
    return m_lut[std::clamp(idx, 0, LUT_SIZE - 1)];
}

uint8_t DensityHeatmap::lerp_u8(uint8_t a, uint8_t b, float t)
{
    float sub = static_cast<float>(b) - static_cast<float>(a);
    return static_cast<uint8_t>(static_cast<float>(a) + t * sub);
}
