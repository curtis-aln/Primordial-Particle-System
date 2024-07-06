#pragma once

#include <SFML/Graphics.hpp>
#include "settings.h"
#include "utils/spatial_hash_grid_array.h"
#include "utils/random.h"
#include "utils/utils.h"


#include <cmath>

#include <array>

inline float dist_squared(const sf::Vector2f position_a, const sf::Vector2f position_b)
{
	const sf::Vector2f delta = position_b - position_a;
	return delta.x * delta.x + delta.y * delta.y;
}


inline void draw_thick_line(sf::RenderWindow& window, const sf::Vector2f& point1, const sf::Vector2f& point2,
	const float thickness = {}, const sf::Color& fill_color = {255, 255, 255})
{
	// Calculate the length and angle of the line
	const float length = std::sqrt(dist_squared(point1, point2));
	const float angle = std::atan2(point2.y - point1.y, point2.x - point1.x) * 180 / pi;

	// Create the rectangle shape
	sf::RectangleShape line(sf::Vector2f(length, thickness));
	line.setOrigin(0, thickness / 2.0f);
	line.setPosition(point1);
	line.setRotation(angle);
	line.setFillColor(fill_color);

	// Draw the line
	window.draw(line);
}

inline void draw_rect_outline(sf::Vector2f top_left, sf::Vector2f bottom_right, sf::RenderWindow& window, const float thickness)
{
	draw_thick_line(window, top_left, { bottom_right.x, top_left.y }, thickness);
	draw_thick_line(window, bottom_right, { top_left.x, bottom_right.y }, thickness);
	draw_thick_line(window, bottom_right, { bottom_right.x, top_left.y }, thickness);
	draw_thick_line(window, top_left, { top_left.x, bottom_right.y }, thickness);
}


constexpr float pi_div_180 = pi / 180.f;
constexpr size_t cos_sin_table_size = 360;
constexpr float rad_sq = SystemSettings::visual_radius * SystemSettings::visual_radius;
constexpr size_t ANGLE_RESOLUTION = 360;
constexpr float ang_res_div_2_pi = ANGLE_RESOLUTION / TWO_PI;


template<size_t population_size>
class ParticlePopulation : SystemSettings
{
	// used to keep particles within bounds
	sf::FloatRect bounds_{};

	alignas(32) std::array<float, ANGLE_RESOLUTION> sin_table;
	alignas(32) std::array<float, ANGLE_RESOLUTION> cos_table;
	alignas(32) std::array<float, population_size> angles;
	
	alignas(32) std::array<sf::Vector2f, population_size> positions;

	alignas(32) std::array<uint16_t, population_size> neighbourhood_count;

	SpatialHashGrid<hash_cells_x, hash_cells_y> hash_grid;

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

		// Initialize sin and cos tables
		for (size_t i = 0; i < ANGLE_RESOLUTION; ++i)
		{
			const float angle = static_cast<float>(i) * 2.0f * pi / ANGLE_RESOLUTION;
			sin_table[i] = std::sin(angle);
			cos_table[i] = std::cos(angle);
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
			calculate_left_and_right(i);
		}

		for (size_t i = 0; i < population_size; ++i)
		{
			// Convert angle to index in the pre-generated tables
			const int angle_index = static_cast<int>(angles[i] * ang_res_div_2_pi) % ANGLE_RESOLUTION;
			sf::Vector2f& position = positions[i];

			position += {gamma * cos_table[angle_index], gamma * sin_table[angle_index]};

			position.x = std::fmod(position.x + bounds_.width, bounds_.width);
			position.y = std::fmod(position.y + bounds_.height, bounds_.height);
		}
	}


	void render(sf::RenderWindow& window, const bool debug = false, const bool draw_hash_grid = false)
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


	void calculate_left_and_right(const size_t index)
	{
		// resetting the particle
		uint8_t l = 0;
		uint8_t r = 0;

		const sf::Vector2f& position = positions[index];
		const c_Vec& near = hash_grid.find(position);

		for (size_t i = 0; i < near.size; i++)
		{
			sf::Vector2f other_position = positions[near.array[i]];

			const sf::Vector2f dir = !near.at_border ? other_position - position : toroidal_direction(other_position - position);
			
			const float dist_sq = dir.x * dir.x + dir.y * dir.y;

			if (dist_sq == 0 || dist_sq > rad_sq)
				continue;


			// Create a unit vector representing the particle's orientation
			const int angle_index = static_cast<int>(angles[index] * ang_res_div_2_pi) % ANGLE_RESOLUTION;

			const bool is_on_right = (dir.x * sin_table[angle_index] - dir.y * cos_table[angle_index]) < 0;

			r += is_on_right;
			l += !is_on_right;
			
		}

		// a + b x N x sign(r-l)
		neighbourhood_count[index] = r + l;
		angles[index] += (alpha + beta * (r + l) * (1 | ((r - l)) >> 31)) * pi_div_180;
	}


	sf::Vector2f toroidal_direction(const sf::Vector2f& direction) const
	{
		return { direction.x - bounds_.width * std::round(direction.x * inv_width),
		direction.y - bounds_.height * std::round(direction.y * inv_height) };
	}



	void render_debug(sf::RenderWindow& window, const size_t index)
	{
		sf::CircleShape p_visual_radius{ visual_radius };
		p_visual_radius.setFillColor({ 0, 0, 0, 0 });
		p_visual_radius.setOutlineThickness(1);
		p_visual_radius.setOutlineColor({ 255 ,255, 255 });

		for (unsigned i = 0; i < population_size; i++)
		{
			//if (i % 3 == 0) // one in three
			//{
			//	p_visual_radius.setPosition(sf::Vector2f{positions_x[index] - visual_radius, positions_y[index] - visual_radius});
			//	window.draw(p_visual_radius);
			//
			//	window.draw(createLine({ positions_x[index], positions_y[index] }, angles[index], 15));
			//}
			
		}

	}

};
