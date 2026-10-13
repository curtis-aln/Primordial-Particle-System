#include "heatmap.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <settings.h>
#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>
#include <utility>
#include <vector>

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
	m_accum.resize(static_cast<size_t>(m_tex_w) * m_tex_h * 3, 0.f);
	m_pixels.resize(static_cast<size_t>(m_tex_w) * m_tex_h * 4, 255u);

	const size_t n = static_cast<size_t>(m_tex_w) * m_tex_h;
	m_pixels.resize(n * 4, 0u);

	if (!m_texture.resize({ m_tex_w, m_tex_h }))
		std::cout << "[DensityHeatmap] Failed to create texture\n";

	// ...after texture resize:
	m_texture.setSmooth(true);

	m_sprite = sf::Sprite(m_texture);

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

void DensityHeatmap::clear()
{
	if (m_trail_decay == 0.f)
		std::fill(m_accum.begin(), m_accum.end(), 0.f);
	else
		for (float& v : m_accum) v *= m_trail_decay;
}

void DensityHeatmap::scatter_particles(const std::vector<float>& pos_x, const std::vector<float>& pos_y,
	const std::vector<sf::Color>& col,
	const std::vector<float>& radii,
	int n, const sf::View& view)
{
	const sf::Vector2f c = view.getCenter();
	const sf::Vector2f vs = view.getSize();
	const float inv_vw = 1.f / vs.x;
	const float inv_vh = 1.f / vs.y;
	const int W = static_cast<int>(m_tex_w);
	const int H = static_cast<int>(m_tex_h);

	const float ts = W * inv_vw;   // texels per world unit
	// particle area in texel^2, times (1 - decay) so steady state == true brightness
	const float area_k = 3.14159265f * ts * ts * (1.f - m_trail_decay);
	constexpr float k255 = 1.f / 255.f;

	for (int i = 0; i < n; ++i)
	{
		const float fx = ((pos_x[i] - c.x) * inv_vw + 0.5f) * W - 0.5f;
		const float fy = ((pos_y[i] - c.y) * inv_vh + 0.5f) * H - 0.5f;

		const int x0 = static_cast<int>(std::floor(fx));
		const int y0 = static_cast<int>(std::floor(fy));
		if (x0 < -1 || x0 >= W || y0 < -1 || y0 >= H) continue;   // off-screen

		const float w = radii[i] * radii[i] * area_k;
		const float r = col[i].r * k255 * w;
		const float g = col[i].g * k255 * w;
		const float b = col[i].b * k255 * w;

		const float sx = fx - x0;
		const float sy = fy - y0;

		auto splat = [&](int x, int y, float k) {
			if (x < 0 || x >= W || y < 0 || y >= H) return;
			float* p = &m_accum[(static_cast<size_t>(y) * W + x) * 3];
			p[0] += r * k;  p[1] += g * k;  p[2] += b * k;
			};

		splat(x0, y0, (1.f - sx) * (1.f - sy));
		splat(x0 + 1, y0, sx * (1.f - sy));
		splat(x0, y0 + 1, (1.f - sx) * sy);
		splat(x0 + 1, y0 + 1, sx * sy);
	}
}

void DensityHeatmap::upload()
{
	auto to_u8 = [](float v) {
		return static_cast<uint8_t>(std::min(v, 1.f) * 255.f + 0.5f);
		};

	const size_t n = m_accum.size() / 3;
	for (size_t i = 0; i < n; ++i)
	{
		m_pixels[i * 4 + 0] = to_u8(m_accum[i * 3 + 0]);
		m_pixels[i * 4 + 1] = to_u8(m_accum[i * 3 + 1]);
		m_pixels[i * 4 + 2] = to_u8(m_accum[i * 3 + 2]);
		m_pixels[i * 4 + 3] = 255u;
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
	m_sprite.setColor(sf::Color(255, 255, 255, alpha));

	sf::RenderStates rs;
	rs.blendMode = sf::BlendAdd;     // must be passed to draw(), not just constructed
	window.draw(m_sprite, rs);       // not window.draw(m_sprite)

	window.setView(saved_view);
}

// ── Tunables ──────────────────────────────────────────────────────────────
void DensityHeatmap::set_gradient(std::vector<GradientStop> stops)
{
	m_stops = std::move(stops);
}

// ── Gradient LUT ──────────────────────────────────────────────────────────


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

uint8_t DensityHeatmap::lerp_u8(uint8_t a, uint8_t b, float t)
{
	float sub = static_cast<float>(b) - static_cast<float>(a);
	return static_cast<uint8_t>(static_cast<float>(a) + t * sub);
}