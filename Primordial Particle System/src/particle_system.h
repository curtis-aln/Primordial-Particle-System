#pragma once

#include <SFML/Graphics.hpp>
#include "settings.h"
#include "utils/spatial_hash_grid.h"
#include "utils/random.h"
#include "utils/utils.h"

#include <cmath>
#include <array>

#include "utils/font.h"

inline sf::Color get_color(const uint16_t total) {
	// Assuming colors is sorted in descending order of first element
	auto it = std::lower_bound(SystemSettings::colors.rbegin(), SystemSettings::colors.rend(), total,
		[](const auto& pair, uint16_t value) { return pair.first > value; });
	return it != SystemSettings::colors.rend() ? it->second : SystemSettings::colors.back().second;
}

class PPS_Renderer {
private:
	sf::VertexArray vertex_array;
	sf::Shader particle_shader;

	// Debug rendering
	sf::RectangleShape visual_radius_shape;
	sf::VertexArray debug_lines;
	Font& debug_font;

	void initializeShader() {
		if (!particle_shader.loadFromMemory(
			"uniform vec2 resolution;"
			"void main() {"
			"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
			"    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;"
			"    gl_FrontColor = gl_Color;"
			"}",
			"uniform vec2 resolution;"
			"void main() {"
			"    vec2 coord = gl_FragCoord.xy / resolution - vec2(0.5);"
			"    if (abs(coord.x) > 0.45 || abs(coord.y) > 0.45) discard;"
			"    gl_FragColor = gl_Color;"
			"}"
		)) {
			throw std::runtime_error("Failed to load particle shader");
		}
	}

public:
	PPS_Renderer(size_t population_size, float particle_size, Font& font)
		: debug_font(font) {
		vertex_array.setPrimitiveType(sf::Quads);
		vertex_array.resize(population_size * 4);

		initializeShader();

		visual_radius_shape.setFillColor(sf::Color::Transparent);
		visual_radius_shape.setOutlineThickness(1);
		visual_radius_shape.setOutlineColor(sf::Color(255, 255, 255, 100));

		debug_lines.setPrimitiveType(sf::Lines);
	}

	void updateParticles(const std::array<sf::Vector2f, SystemSettings::particle_count>& positions,
		const std::array<uint16_t, SystemSettings::particle_count>& neighbourhood_count,
		float particle_size) {
		for (size_t i = 0; i < positions.size(); ++i) {
			const sf::Vector2f& center = positions[i];
			const sf::Color color = get_color(neighbourhood_count[i]);
			const float half_size = particle_size / 2.f;

			sf::Vertex* quad = &vertex_array[i * 4];
			quad[0].position = center + sf::Vector2f(-half_size, -half_size);
			quad[1].position = center + sf::Vector2f(half_size, -half_size);
			quad[2].position = center + sf::Vector2f(half_size, half_size);
			quad[3].position = center + sf::Vector2f(-half_size, half_size);

			quad[0].color = quad[1].color = quad[2].color = quad[3].color = color;
		}
	}

	void render(sf::RenderWindow& window) {
		sf::RenderStates states;
		states.shader = &particle_shader;
		particle_shader.setUniform("resolution", sf::Glsl::Vec2(window.getSize()));

		window.draw(vertex_array, states);
	}

	void renderDebug(sf::RenderWindow& window,
		const std::array<sf::Vector2f, SystemSettings::particle_count>& positions,
		const std::array<float, SystemSettings::particle_count>& angles,
		const std::array<uint16_t, SystemSettings::particle_count>& neighbourhood_count,
		const std::array<cellIndex, SystemSettings::particle_count>& cell_indexes,
		float visual_radius, float particle_size) {
		debug_lines.clear();
		debug_lines.resize(positions.size() * 2);

		for (size_t i = 0; i < positions.size(); ++i) {
			const sf::Vector2f& position = positions[i];
			float angle = angles[i];

			// Draw direction line
			sf::Vector2f direction = sf::Vector2f(std::sin(angle), std::cos(angle)) * particle_size;
			debug_lines[i * 2].position = position;
			debug_lines[i * 2].color = sf::Color::White;
			debug_lines[i * 2 + 1].position = position + direction;
			debug_lines[i * 2 + 1].color = sf::Color::White;

			// Draw visual radius
			visual_radius_shape.setSize(sf::Vector2f(visual_radius * 2, visual_radius * 2));
			visual_radius_shape.setPosition(position - sf::Vector2f(visual_radius, visual_radius));
			window.draw(visual_radius_shape, sf::BlendAdd);

			// Draw debug text
			sf::Vector2f text_pos = position + sf::Vector2f(0, 30);
			debug_font.draw(text_pos, "nearby: " + std::to_string(neighbourhood_count[i]), true);
			text_pos.y += 30;
			debug_font.draw(text_pos, "at cell: (" + std::to_string(cell_indexes[i].first) + ", " + std::to_string(cell_indexes[i].second) + ")", true);
		}

		window.draw(debug_lines);
	}
};



// pre-calculating variables
inline static constexpr float rad_sq = SystemSettings::visual_radius * SystemSettings::visual_radius;


template<size_t population_size>
class ParticlePopulation : SystemSettings
{
	alignas(32) std::array<sf::Vector2f, population_size> positions;
	alignas(32) std::array<float, population_size> angles;

	alignas(32) std::array<uint16_t, population_size> neighbourhood_count;

	alignas(32) std::array<cellIndex, population_size> cell_indexes;

	SpatialHashGrid<hash_cells_x, hash_cells_y> hash_grid;

	PPS_Renderer renderer;

	// rendering and graphics
	sf::RectangleShape render_bounds{};

	const float inv_width = 0.f;
	const float inv_height = 0.f;

	

public:
	ParticlePopulation(Font& debug_font)
	: hash_grid({0, 0, world_width, world_height}),
	inv_width(1.f / world_width), inv_height(1.f / world_height),
		renderer(population_size, radius, debug_font)
	{
		// initializing particle
		for (size_t i = 0; i < population_size; ++i)
		{
			positions[i] = Random::rand_pos_in_rect(sf::FloatRect{0, 0, world_width, world_height});
			angles[i] = Random::rand_range(0.f, 2.f * pi);
		}
	}


	void add_particles_to_grid()
	{
		hash_grid.clear();
		for (size_t i = 0; i < population_size; ++i)
		{
			sf::Vector2f& position = positions[i];
			position.x = std::fmod(position.x + world_width, world_width);
			position.y = std::fmod(position.y + world_height, world_height);

			cell_indexes[i] = hash_grid.add_object(position, i);
		}
	}

	void update_particles(const bool paused = false)
	{
		// filling the left and right arrays with data
		for (size_t i = 0; i < population_size; ++i)
		{
			new_update_angles_optimized(i, paused);
		}
	}



	void render(sf::RenderWindow& window, const bool draw_hash_grid = false, sf::Vector2f pos = {0 ,0})
	{
		positions[0] = pos;
		if (draw_hash_grid)
		{
			hash_grid.render_grid(window);
		}

		render_particles(window);

		draw_rect_outline({ 0, 0 }, {world_width, world_height}, window, 30);
	}


	void render_particles(sf::RenderWindow& window)
	{
		renderer.updateParticles(positions, neighbourhood_count, radius);
		renderer.render(window);
	}

	void render_debug(sf::RenderWindow& window)
	{
		renderer.renderDebug(window, positions, angles, neighbourhood_count, cell_indexes, visual_radius, radius);
	}


private:
	inline void new_update_angles_optimized(const size_t index, const bool paused)
	{
		// first fetch the data we need
		sf::Vector2f& position = positions[index];

		float& angle = angles[index];

		// then pre-calculate the sin and cos values for the angle
		const float sin_angle = std::sin(angle);
		const float cos_angle = std::cos(angle);

		// finds the nearby indexes in each of the 9 neighbouring cells
		hash_grid.find(position);

		// calculating the cumulative direction, does not need to be average to function the same
		int left = 0;
		int right = 0;

		for (int i = 0; i < hash_grid.found_array_size; ++i)
		{
			const sf::Vector2f other_position = positions[hash_grid.found_array[i]];
			sf::Vector2f direction_to = other_position - position;
			if (hash_grid.at_border)
			{
				direction_to = toroidal_direction(direction_to);
			}

			const float dist_sq = direction_to.x * direction_to.x + direction_to.y * direction_to.y;

			if (dist_sq > 0 && dist_sq < rad_sq)
			{
				const bool is_on_right = (direction_to.x * sin_angle - direction_to.y * cos_angle) < 0;
				right += is_on_right;
				left += !is_on_right;
			}
		}

		// checking if the direction is on the right of the particle, if so converting this into -1 for false and 1 for trie
		const float r_minus_l = right - left;
		const float sign = r_minus_l >= 0 ? 1 : -1;

		neighbourhood_count[index] = right + left;

		// updating the angle
		angle += (alpha + beta * (right + left) * sign) * pi_div_180;

		if (paused)
		{
			return;
		}

		// updating the position
		position += {gamma* cos_angle, gamma* sin_angle};
	}

	inline sf::Vector2f toroidal_direction(const sf::Vector2f& direction) const
	{
		return { // todo check using int instead of round
			direction.x - world_width * std::round(direction.x * inv_width),
			direction.y - world_height * std::round(direction.y * inv_height)
		};
	}
};
