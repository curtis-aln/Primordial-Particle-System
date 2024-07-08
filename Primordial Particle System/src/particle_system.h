#pragma once

#include <SFML/Graphics.hpp>
#include "settings.h"
#include "utils/spatial_hash_grid_array.h"
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
	// used to keep particles within bounds
	sf::FloatRect bounds_{};

	alignas(32) std::array<sf::Vector2f, population_size> positions;
	alignas(32) std::array<float, population_size> angles;

	alignas(32) std::array<uint16_t, population_size> neighbourhood_count;

	SpatialHashGridOptimized<hash_cells_x, hash_cells_y> hash_grid;

	// rendering and graphics
	sf::CircleShape circle_drawer{};
	sf::RectangleShape render_bounds{};

	const float inv_width{};
	const float inv_height{};


public:
	ParticlePopulation(const sf::FloatRect& bounds) : bounds_(bounds), hash_grid(bounds), inv_width(1.f / bounds_.width), inv_height(1.f / bounds_.height)
	{
		// initializing particle
		for (size_t i = 0; i < population_size; ++i)
		{
			positions[i] = Random::rand_pos_in_rect(bounds);
			angles[i] = Random::rand_range(0.f, 2.f * pi);
		}

		// initializing renderer
		circle_drawer.setRadius(radius);
		circle_drawer.setPointCount(20);
	}


	void update_vectorized()
	{
		// resetting updating the spatial hash grid
		hash_grid.clear();
		for (size_t i = 0; i < population_size; ++i)
		{
			hash_grid.add_object(positions[i], i);
		}

		// filling the left and right arrays with data
		for (size_t i = 0; i < population_size; ++i)
		{
			update_angles_optimized(i);
		}
	}


	void render(sf::RenderWindow& window, const bool draw_hash_grid = false)
	{
		if (draw_hash_grid)
		{
			window.draw(hash_grid.vertexBuffer);
		}

		for (size_t i = 0; i < population_size; ++i)
		{
			circle_drawer.setFillColor(get_color(i));
			circle_drawer.setPosition(positions[i] - sf::Vector2f{radius, radius});
			window.draw(circle_drawer, sf::BlendAdd);
		}

		draw_rect_outline(bounds_.getPosition(), bounds_.getPosition() + bounds_.getSize(), window, 4);
	}

	void render_debug(sf::RenderWindow& window, Font& font)
	{
		sf::CircleShape p_visual_radius{ visual_radius };
		p_visual_radius.setFillColor({ 0, 0, 0, 0 });
		p_visual_radius.setOutlineThickness(1);
		p_visual_radius.setOutlineColor({ 255 ,255, 255, 100 });

		for (unsigned i = 0; i < population_size; i++)
		{
			const float angle = angles[i];
			const sf::Vector2f position = positions[i];
			const sf::Vector2f direction = { sin(angle) * radius, cos(angle) * radius };

			draw_thick_line(window, positions[i], positions[i] + direction, 0.5f);

			p_visual_radius.setPosition(position - sf::Vector2f{ visual_radius, visual_radius });
			window.draw(p_visual_radius, sf::BlendAdd);

			// drawing the amount of neighbours the particle has
			const sf::Vector2f offset = { 20, 20 };
			font.draw(position + offset, std::to_string(neighbourhood_count[i]), true);
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


	inline void update_angles_optimized(const size_t index)
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

			const sf::Vector2f dir = !hash_grid.at_border ? other_position - position : toroidal_direction(other_position - position);
			const float dist_sq = dir.x * dir.x + dir.y * dir.y;

			const float conditions = (dist_sq != 0 && dist_sq < rad_sq);
			nearby += conditions;
			cumulative_dir += dir * conditions;
		}

		// checking if the direction is on the right of the particle, if so converting this into -1 for false and 1 for trie
		const bool is_on_right = (cumulative_dir.x * sin_angle - cumulative_dir.y * cos_angle) < 0;
		const float resized = 2 * is_on_right - 1;

		neighbourhood_count[index] = nearby;

		// updating the angle
		angle += (alpha + beta * nearby * resized) * pi_div_180;

		// updating the position
		position += {gamma * cos_angle, gamma * sin_angle};

		position.x = std::fmod(position.x + bounds_.width, bounds_.width);
		position.y = std::fmod(position.y + bounds_.height, bounds_.height);
	}



	inline sf::Vector2f toroidal_direction(const sf::Vector2f& direction) const
	{
		return { // todo check using int instead of round
			direction.x - bounds_.width * std::round(direction.x * inv_width),
			direction.y - bounds_.height * std::round(direction.y * inv_height)
		};
	}
};
