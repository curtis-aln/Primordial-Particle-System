// density_heatmap.h
#pragma once


#include "../settings.h"
#include <cstdint>
#include <cstdint>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/View.hpp>
#include <vector>

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

	void upload();
	void scatter_particles(const std::vector<float>& pos_x, const std::vector<float>& pos_y,
		const std::vector<sf::Color>& col,
		const std::vector<float>& radii,
		int n, const sf::View& view);

	// alpha: 0 = invisible, 255 = fully opaque
	void draw(sf::RenderWindow& window, uint8_t alpha = 255);

	// ── Tunables ──────────────────────────────────────────────────────────────

	struct GradientStop { float t; sf::Color colour; };

	void set_gradient(std::vector<GradientStop> stops);

private:
	sf::Color interpolate_gradient(float t) const;

	static uint8_t lerp_u8(uint8_t a, uint8_t b, float t);

	// ── Members ───────────────────────────────────────────────────────────────
	float        m_world_w, m_world_h;
	unsigned int m_tex_w, m_tex_h;
	float        m_inv_world_x, m_inv_world_y;
	float        m_trail_decay = 0.f;  // 0 = disabled

	std::vector<float> m_accum;   // RGB interleaved: sum of particle colour * coverage

	std::vector<uint8_t>  m_pixels;
	sf::Texture           m_texture;
	sf::Sprite            m_sprite;

	std::vector<GradientStop>       m_stops;
};