#pragma once

#include <SFML/Graphics.hpp>
#include "settings.h"
#include "utils/spatial_hash_grid.h"
#include "utils/random.h"
#include "utils/utils.h"

#include <cmath>
#include <array>

#include "utils/font.h"


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

	// rendering and graphics
	sf::CircleShape circle_drawer{};
	sf::RectangleShape render_bounds{};

	const float inv_width{};
	const float inv_height{};


public:
	ParticlePopulation() : hash_grid({0, 0, world_width, world_height}), inv_width(1.f / world_width), inv_height(1.f / world_height)
	{
		// initializing particle
		for (size_t i = 0; i < population_size; ++i)
		{
			positions[i] = Random::rand_pos_in_rect(sf::FloatRect{0, 0, world_width, world_height});
			angles[i] = Random::rand_range(0.f, 2.f * pi);
		}

		// initializing renderer
		circle_drawer.setRadius(radius);
		circle_drawer.setPointCount(20);
	}


	void add_particles_to_grid()
	{
		hash_grid.clear();
		for (size_t i = 0; i < population_size; ++i)
		{
			cell_indexes[i] = hash_grid.add_object(positions[i], i);
		}
	}

	void update_particles(const bool paused = false)
	{
		// filling the left and right arrays with data
		for (size_t i = 0; i < population_size; ++i)
		{
			update_angles_optimized(i, paused);
		}
	}



	void render(sf::RenderWindow& window, const bool draw_hash_grid = false, sf::Vector2f pos = {0 ,0})
	{
		if (pos.x > 0 && pos.y > 0 && pos.x < world_width && pos.y < world_height)
		{
			for (int i = 0; i < 1; ++i)
			{
				positions[i] = pos + Random::rand_vector(-50.f, 50.f);
			}
		}

		//for (int i = 0; i < particle_count; ++i)
		//{
		//	positions[i].x += 1.f;
		//}

		if (draw_hash_grid)
		{
			hash_grid.render_grid(window);
		}

		for (size_t i = 0; i < population_size; ++i)
		{
			circle_drawer.setFillColor(get_color(i));
			circle_drawer.setPosition(positions[i] - sf::Vector2f{radius, radius});
			window.draw(circle_drawer, sf::BlendAdd);
		}

		draw_rect_outline({ 0, 0 }, {world_width, world_height}, window, 20);
	}

	void render_debug(sf::RenderWindow& window, Font& font)
	{
		sf::CircleShape p_visual_radius{ visual_radius };
		p_visual_radius.setFillColor({ 0, 0, 0, 0 });
		p_visual_radius.setOutlineThickness(8);
		p_visual_radius.setOutlineColor({ 255 ,255, 255, 100 });

		for (unsigned i = 0; i < population_size; i++)
		{
			const float angle = angles[i];
			const sf::Vector2f position = positions[i];
			const sf::Vector2f direction = { sin(angle) * radius, cos(angle) * radius };

			draw_thick_line(window, positions[i], positions[i] + direction, 8.f);

			p_visual_radius.setPosition(position - sf::Vector2f{ visual_radius, visual_radius });
			window.draw(p_visual_radius, sf::BlendAdd);

			// drawing the amount of neighbours the particle has
			const sf::Vector2f offset = { 0, 30 };

			const auto neighbours = std::to_string(neighbourhood_count[i]);
			font.draw(position + offset, "nearby: " + neighbours, true);

			// draw the grid cell the particle is in
			const auto cellX = std::to_string(cell_indexes[i].first);
			const auto cellY = std::to_string(cell_indexes[i].second);
			font.draw(position + offset * 2.f, "at cell: (" + cellX + ", " + cellY + ")", true);
		}

	}


private:
	sf::Color get_color(const size_t index)
	{
		const uint16_t total = neighbourhood_count[index];

		// Assuming colors is sorted in descending order of first element
		auto it = std::lower_bound(colors.rbegin(), colors.rend(), total,
			[](const auto& pair, uint16_t value) {
				return pair.first > value;
			});

		return it != colors.rend() ? it->second : colors.back().second;
	}


	inline void update_angles_optimized(const size_t index, const bool paused)
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
		sf::Vector2f cumulative_dir = {0, 0};
		int nearby = 0;

		for (int i = 0; i < hash_grid.found_array_size; ++i)
		{
			const sf::Vector2f other_position = positions[hash_grid.found_array[i]];
			sf::Vector2f direction_to = other_position - position;
			if (hash_grid.at_border)
			{
				direction_to = toroidal_direction(direction_to);
			}

			const float dist_sq = direction_to.x * direction_to.x + direction_to.y * direction_to.y;

			const float conditions = (dist_sq > 0 && dist_sq < rad_sq);

			nearby += conditions;
			cumulative_dir += direction_to * conditions;
		}

		// checking if the direction is on the right of the particle, if so converting this into -1 for false and 1 for trie
		const bool is_on_right = (cumulative_dir.x * sin_angle - cumulative_dir.y * cos_angle) < 0;
		const float resized = 2 * is_on_right - 1;

		neighbourhood_count[index] = nearby;

		// updating the angle
		angle += (alpha + beta * nearby * resized) * pi_div_180;

		if (paused)
		{
			return;
		}

		// updating the position
		position += {gamma * cos_angle, gamma * sin_angle};

		position.x = std::fmod(position.x + world_width, world_width);
		position.y = std::fmod(position.y + world_height, world_height);
	}



	inline sf::Vector2f toroidal_direction(const sf::Vector2f& direction) const
	{
		return { // todo check using int instead of round
			direction.x - world_width * std::round(direction.x * inv_width),
			direction.y - world_height * std::round(direction.y * inv_height)
		};
	}
};
