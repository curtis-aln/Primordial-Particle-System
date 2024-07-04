#pragma once

#include <SFML/Graphics.hpp>
#include "settings.h"
#include "utils/spatial_hash_grid.h"
#include "utils/random.h"
#include "utils/utils.h"


#include <cmath>
#include <random>

#include <immintrin.h> // For AVX instructions
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
constexpr float ang_res_div_2pi = ANGLE_RESOLUTION / TWO_PI;


template<size_t population_size>
class ParticlePopulation : SystemSettings
{
	// used to keep particles within bounds
	sf::FloatRect bounds_{};

	alignas(32) std::array<float, ANGLE_RESOLUTION> sin_table;
	alignas(32) std::array<float, ANGLE_RESOLUTION> cos_table;

	
	// Use Structure of Arrays for better cache utilization
	alignas(32) std::vector<float> positions_x;
	alignas(32) std::vector<float> positions_y;
	alignas(32) std::vector<float> angles;
	alignas(32) std::vector<uint8_t> left;
	alignas(32) std::vector<uint8_t> right;
	alignas(32) std::vector<cell_idx> cell_indexes;

	SpatialHashGrid<hash_cells_x, hash_cells_y> hash_grid;
	sf::Vector2f hash_cell_size;

	// rendering and graphics
	sf::CircleShape circle_drawer{};
	sf::RectangleShape render_bounds{};


public:
	ParticlePopulation(const sf::FloatRect& bounds) : bounds_(bounds), hash_grid(bounds)
	{
		positions_x.resize(population_size);
		positions_y.resize(population_size);
		angles.resize(population_size);
		left.resize(population_size);
		right.resize(population_size);
		cell_indexes.resize(population_size);

		// initializing particle
		for (size_t i = 0; i < population_size; ++i)
		{
			sf::Vector2f pos = Random::rand_pos_in_rect(bounds);
			positions_x[i] = pos.x;
			positions_y[i] = pos.y;
			angles[i] = Random::rand_range(0.f, 2.f * pi);
			left[i] = 0;
			right[i] = 0;
		}

		// Initialize sin and cos tables
		for (size_t i = 0; i < ANGLE_RESOLUTION; ++i)
		{
			float angle = static_cast<float>(i) * 2.0f * pi / ANGLE_RESOLUTION;
			sin_table[i] = std::sin(angle);
			cos_table[i] = std::cos(angle);
		}


		// initializing renderer
		circle_drawer.setRadius(radius);
	}


	void update()
	{
		// resetting updating the spatial hash grid
		hash_grid.clear();
		for (size_t i = 0; i < population_size; ++i)
		{
			cell_indexes[i] = hash_grid.add_object({ positions_x[i], positions_y[i]}, i);
		}

		// for each particle we calculate its neighbours
		for (size_t i = 0; i < population_size; ++i)
		{
			calculate_neighbours(i);
			update_particle_positioning(i);
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
			circle_drawer.setPosition({ positions_x[i] - radius, positions_y[i] - radius });
			window.draw(circle_drawer, sf::BlendAdd);
		}

		draw_rect_outline(bounds_.getPosition(), bounds_.getPosition() + bounds_.getSize(), window, 4);
	}


private:
	sf::Color get_color(const size_t index)
	{
		const uint16_t total = left[index] + right[index];

		// Assuming colors is sorted in descending order of first element
		auto it = std::lower_bound(colors.rbegin(), colors.rend(), total,
			[](const auto& pair, uint16_t value) {
				return pair.first > value;
			});

		return it != colors.rend() ? it->second : colors.back().second;
	}


	void calculate_neighbours(const size_t index)
	{
		// resetting the particle
		uint8_t l = 0;
		uint8_t r = 0;

		const sf::Vector2f& position = { positions_x[index], positions_y[index] };
		const c_Vec& near = hash_grid.find(cell_indexes[index]);

		for (size_t i = 0; i < near.size; i++)
		{
			const obj_idx other_index = near.array[i];

			sf::Vector2f other_position = {positions_x[other_index], positions_y[other_index]};

			sf::Vector2f dir = near.at_border ? toroidal_direction(position, other_position, bounds_) : other_position - position;
			float dist_sq = dir.x * dir.x + dir.y * dir.y;


			if (dist_sq > 0 && dist_sq < rad_sq)
			{
				// Calculate the x and y coordinates of other_pos relative to pos
				const bool is_on_right = isOnRightHemisphere(dir, angles[index]);

				r += is_on_right;
				l += !is_on_right;
			}
		}

		left[index]  = l;
		right[index] = r;
	}


	void update_particle_positioning(const size_t index)
	{
		float& pos_x = positions_x[index];
		float& pos_y = positions_y[index];
		float& angle = angles[index];

		// delta_phi = alpha + beta × N × sign(R - L)
		const unsigned sum = right[index] + left[index];

		//const float act_sum = sum <= activation ? sum : 0;
		
		const float delta = alpha + beta * sum * sign(right[index] - left[index]); // calculate_b(left[index], right[index]);
		angle += delta * pi_div_180;

		// Convert angle to index in the pre-generated tables
		const int angle_index = static_cast<int>(angle * ang_res_div_2pi) % ANGLE_RESOLUTION;

		pos_x += gamma * cos_table[angle_index];
		pos_y += gamma * sin_table[angle_index];

		pos_x = std::fmod(pos_x + bounds_.width, bounds_.width);
		pos_y = std::fmod(pos_y + bounds_.height, bounds_.height);
	}




	void render_debug(sf::RenderWindow& window, const size_t index)
	{
		sf::CircleShape p_visual_radius{ visual_radius };
		p_visual_radius.setFillColor({ 0, 0, 0, 0 });
		p_visual_radius.setOutlineThickness(1);
		p_visual_radius.setOutlineColor({ 255 ,255, 255 });

		for (unsigned i = 0; i < population_size; i++)
		{
			if (i % 3 == 0) // one in three
			{
				p_visual_radius.setPosition(sf::Vector2f{positions_x[index] - visual_radius, positions_y[index] - visual_radius});
				window.draw(p_visual_radius);

				window.draw(createLine({ positions_x[index], positions_y[index] }, angles[index], 15));
			}
			
		}

	}

};

// 82