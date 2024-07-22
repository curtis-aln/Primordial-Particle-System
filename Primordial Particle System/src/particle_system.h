﻿#pragma once

#include <SFML/Graphics.hpp>
#include "settings.h"
#include "utils/spatial_hash_grid.h"
#include "utils/random.h"

#include <cmath>
#include <array>

#include "PPS_renderer.h"

#include <vector>
#include <omp.h>        // For OpenMP parallelization


inline static constexpr float pi = 3.141592653589793238462643383279502884197f;
inline static constexpr float two_pi = 2.f * pi;
inline static constexpr float pi_div_180 = pi / 180.f;


template<size_t PopulationSize>
class ParticlePopulation : PPS_Settings
{
	// Aligned memory allocation for better vectorization
	alignas(32) std::vector<float> positions_x_;
	alignas(32) std::vector<float> positions_y_;
	alignas(32) std::vector<float> angles_;
	alignas(32) std::vector<uint16_t> neighbourhood_count_;

	// Pre-computed constants for fast lookup
	static constexpr int ANGLE_TABLE_SIZE = 256;
	alignas(32) float sin_table_[ANGLE_TABLE_SIZE];
	alignas(32) float cos_table_[ANGLE_TABLE_SIZE];


	// The Spatial Hash Grid Optimizes finding who is nearby
	SpatialGrid<grid_cells_x, grid_cells_y> spatial_grid;

	// renders the pps
	PPS_Renderer pps_renderer_;

	// rendering and graphics
	sf::RectangleShape render_bounds_{};

	// pre-computed variables
	float inv_width_ = 0.f;
	float inv_height_ = 0.f;

	std::array<int, cell_capacity * 9> neighbours;
	int neighbours_size = 0;


public:
	explicit ParticlePopulation(sf::RenderWindow& window)
	: spatial_grid({0, 0, world_width, world_height}),
	  pps_renderer_(PopulationSize, window),
	  inv_width_(1.f / world_width),
	  inv_height_(1.f / world_height)
	{

		positions_x_.resize(PopulationSize);
		positions_y_.resize(PopulationSize);
		angles_.resize(PopulationSize);
		neighbourhood_count_.resize(PopulationSize);

		// Initialize sin and cos tables
		for (int i = 0; i < ANGLE_TABLE_SIZE; ++i) {
			float angle = (i / static_cast<float>(ANGLE_TABLE_SIZE)) * two_pi;
			sin_table_[i] = std::sin(angle);
			cos_table_[i] = std::cos(angle);
		}

		// initializing particle with random initial states
		for (size_t i = 0; i < PopulationSize; ++i)
		{
			sf::FloatRect bounds = { 0, 0, world_width, world_height };
			sf::Vector2f center = { world_width / 2, world_height / 2 };
			//positions_[i] = Random::rand_pos_in_circle(center, 500);
			const sf::Vector2f pos = Random::rand_pos_in_rect(bounds);
			positions_x_[i] = pos.x;
			positions_y_[i] = pos.y;
			angles_[i] = Random::rand_range(0.f, 2.f * pi); // radians
		}
	}


	void add_particles_to_grid()
	{
		// At the start of every iteration. all the particles need to be removed from the grid and re-added
		spatial_grid.clear();
		for (size_t i = 0; i < PopulationSize; ++i)
		{
			// positions are fetched and wrapped
			float& x = positions_x_[i];
			float& y = positions_y_[i];

			x = std::fmod(x + world_width, world_width);
			y = std::fmod(y + world_height, world_height);

			spatial_grid.add_object({x, y}, i);
		}
	}

	void update_particles(const bool paused = false)
	{
		// filling the left and right arrays with data
		for (size_t i = 0; i < grid_cells_x * grid_cells_y; ++i)
		{
			process_cell(i);
		}
	}


	void render(sf::RenderWindow& window, const bool draw_hash_grid = false, const sf::Vector2f pos = {0 ,0})
	{
		//positions_[0] = pos;
		if (draw_hash_grid)
		{
			spatial_grid.render_grid(window);
		}

		pps_renderer_.updateParticles(positions_x_, positions_y_, neighbourhood_count_);
		pps_renderer_.render();
	}


	void render_debug(sf::RenderWindow& window, const sf::Vector2f mouse_pos, const float debug_radius)
	{
		pps_renderer_.render_debug(positions_x_, positions_y_, angles_, neighbourhood_count_, mouse_pos, debug_radius);
	}


private:
	inline void process_cell(const cell_idx cell_index)
	{
		// for a given cell this function will access its particle contents. and for each one of them it will update them based off the information from the
		// neighbouring 9 cells.
		neighbours_size = 0;

		const int cell_index_x = cell_index % grid_cells_x;
		const int cell_index_y = cell_index / grid_cells_x;
		const bool at_border = cell_index_x == 0 || cell_index_y == 0 || cell_index_x == grid_cells_x - 1 || cell_index_y == grid_cells_y - 1;

		for (int dx = -1; dx <= 1; ++dx)
		{
			for (int dy = -1; dy <= 1; ++dy)
			{
				// calculating the location of the neighbour
				int32_t neighbour_index_x = cell_index_x + dx;
				int32_t neighbour_index_y = cell_index_y + dy;

				if (at_border)
				{
					neighbour_index_x = (neighbour_index_x + grid_cells_x) % grid_cells_x;
					neighbour_index_y = (neighbour_index_y + grid_cells_y) % grid_cells_y;
				}

				// processing the neighbour
				const uint32_t neighbour_index = neighbour_index_y * grid_cells_x + neighbour_index_x;
				const auto& contents = spatial_grid.grid[neighbour_index];
				const auto size = spatial_grid.objects_count[neighbour_index];

				// adding the neighbour data to the array
				std::copy_n(contents.begin(), size, neighbours.begin() + neighbours_size);
				neighbours_size += size;
			}
		}

		// updating th particles
		const auto& cell_contents = spatial_grid.grid[cell_index];
		const uint8_t cell_size = spatial_grid.objects_count[cell_index];

		for (uint8_t idx = 0; idx < cell_size; ++idx)
		{
			update_particle(cell_contents[idx], at_border);
		}

	}


	inline void update_particle(const obj_idx index, const bool at_border)
	{
		// first fetch the data we need
		float& x = positions_x_[index];
		float& y = positions_y_[index];
		float& angle = angles_[index];

		// Convert angle to lookup table index
		const int angle_index = static_cast<int>((angle / two_pi) * ANGLE_TABLE_SIZE) & (ANGLE_TABLE_SIZE - 1);
		const float sin_angle = sin_table_[angle_index];
		const float cos_angle = cos_table_[angle_index];

		int left = 0;
		int right = 0;
		for (uint32_t i{ 0 }; i < neighbours_size; ++i)
		{
			const uint32_t other_particle = neighbours[i];

			float to_x = positions_x_[other_particle] - x;
			float to_y = positions_y_[other_particle] - y;

			if (at_border)
			{
				to_x -= world_width * std::round(to_x * inv_width_);
				to_y -= world_height * std::round(to_y * inv_height_);
			}

			const float dist_sq = to_x * to_x + to_y * to_y;

			if (dist_sq > 0 && dist_sq < visual_radius * visual_radius)
			{
				const bool is_on_right = (to_x * sin_angle - to_y * cos_angle) < 0;
				right += is_on_right;
				left += !is_on_right;
			}
		}

		// checking if the direction is on the right of the particle, if so converting this into -1 for false and 1 for trie
		const auto sign = static_cast<float>((right - left) >= 0 ? 1 : -1);
		neighbourhood_count_[index] = right + left;

		angle += (alpha + beta * (right + left) * sign) * pi_div_180;

		// Update position using the look-up table
		x += gamma * cos_angle;
		y += gamma * sin_angle;
	}
};
