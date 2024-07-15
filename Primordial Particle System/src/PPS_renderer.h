#pragma once
#include "settings.h"


inline sf::Color get_color(const uint16_t total)
{
	// Assuming colors is sorted in descending order of first element
	const auto it = std::lower_bound(PPS_Graphics::colors.rbegin(), PPS_Graphics::colors.rend(), total,
	                                 [](const auto& pair, uint16_t value) { return pair.first > value; });
	return it != PPS_Graphics::colors.rend() ? it->second : PPS_Graphics::colors.back().second;
}


class PPS_Renderer : PPS_Graphics
{
private:
	sf::VertexArray vertex_array_;
	sf::Shader particle_shader_;

	// Debug rendering
	sf::CircleShape visual_radius_shape_;
	sf::VertexArray debug_lines_;
	Font& debug_font_;

	void initializeShader()
	{
		if (!particle_shader_.loadFromMemory(
			"uniform vec2 resolution;"
			"void main() {"
			"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
			"    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;"
			"    gl_FrontColor = gl_Color;"
			"}",
			"uniform vec2 resolution;"
			"void main() {"
			"    vec2 coord = gl_FragCoord.xy / resolution - vec2(0.5);"
			"    if (abs(coord.x) > 0.55 || abs(coord.y) > 0.55) discard;"
			"    gl_FragColor = gl_Color;"
			"}"
		)) {
			throw std::runtime_error("Failed to load particle shader");
		}
	}

public:
	PPS_Renderer(const size_t population_size, Font& font) : debug_font_(font)
	{
		vertex_array_.setPrimitiveType(sf::Quads);
		vertex_array_.resize(population_size * 4);

		initializeShader();

		visual_radius_shape_.setFillColor(sf::Color::Transparent);
		visual_radius_shape_.setOutlineThickness(5);
		visual_radius_shape_.setOutlineColor(sf::Color(255, 255, 255, 100));
		visual_radius_shape_.setRadius(PPS_Settings::visual_radius);

		debug_lines_.setPrimitiveType(sf::Lines);
	}

	void updateParticles(const std::vector<sf::Vector2f>& positions,
		const std::vector<uint16_t>& neighbourhood_count)
	{
		for (size_t i = 0; i < positions.size(); ++i) 
		{
			const sf::Vector2f& center = positions[i];
			const sf::Color color = get_color(neighbourhood_count[i]);
			const float half_size = particle_radius / 2.f;

			sf::Vertex* quad = &vertex_array_[i * 4];
			quad[0].position = center + sf::Vector2f(-half_size, -half_size);
			quad[1].position = center + sf::Vector2f(half_size, -half_size);
			quad[2].position = center + sf::Vector2f(half_size, half_size);
			quad[3].position = center + sf::Vector2f(-half_size, half_size);

			quad[0].color = quad[1].color = quad[2].color = quad[3].color = color;
		}
	}

	void render(sf::RenderWindow& window)
	{
		sf::RenderStates states;
		states.shader = &particle_shader_;
		particle_shader_.setUniform("resolution", sf::Glsl::Vec2(window.getSize()));

		window.draw(vertex_array_, states);
	}


	void render_debug(sf::RenderWindow& window, const std::vector<sf::Vector2f>& positions, const std::vector<float>& angles,
		const std::vector<uint16_t>& neighbourhood_count, const sf::Vector2f mouse_pos, const float mouse_radius)
	{
		constexpr float visual_radius = PPS_Settings::visual_radius;

		debug_lines_.clear();
		debug_lines_.resize(positions.size() * 2);

		for (size_t i = 0; i < positions.size(); ++i) 
		{
			const sf::Vector2f& position = positions[i];

			if (dist_squared(position, mouse_pos) > mouse_radius * mouse_radius)
			{
				continue;
			}

			const float angle = angles[i];

			// Draw direction line
			sf::Vector2f direction = sf::Vector2f(std::sin(angle), std::cos(angle)) * particle_radius;
			debug_lines_[i * 2].position = position;
			debug_lines_[i * 2].color = sf::Color::White;
			debug_lines_[i * 2 + 1].position = position + direction;
			debug_lines_[i * 2 + 1].color = sf::Color::White;

			// Draw visual radius
			visual_radius_shape_.setPosition(position - sf::Vector2f(visual_radius, visual_radius));
			window.draw(visual_radius_shape_, sf::BlendAdd);

			// Draw debug text
			sf::Vector2f text_pos = position + sf::Vector2f(0, particle_radius);
			debug_font_.draw(text_pos, "nearby: " + std::to_string(neighbourhood_count[i]), true);
			debug_font_.draw(text_pos + sf::Vector2f(0, 30), "angle: " + std::to_string(angles[i]), true);
		}

		window.draw(debug_lines_);
	}
};
