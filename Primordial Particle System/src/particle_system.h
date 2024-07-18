#pragma once

#include <SFML/Graphics.hpp>
#include "settings.h"
#include "utils/spatial_hash_grid.h"
#include "utils/random.h"

#include <cmath>
#include <array>

#include "utils/font.h"
#include "PPS_renderer.h"

#include <immintrin.h>  // For SSE/AVX intrinsics
#include <algorithm>    // For std::fill
#include <vector>
#include <omp.h>        // For OpenMP parallelization

#ifdef __llvm__
#define PREFETCH(addr) __builtin_prefetch(addr, 0, 3)
#else
#define PREFETCH(addr) // Do nothing for other compilers
#endif


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

	// Pre-computed constants
	alignas(32) float sin_table_[256];
	alignas(32) float cos_table_[256];


	// The Spatial Hash Grid Optimizes finding who is nearby
	SpatialHashGrid<hash_cells_x, hash_cells_y> hash_grid_;

	PPS_Renderer renderer_;

	// rendering and graphics
	sf::RectangleShape render_bounds_{};

	// pre-computed variables
	float inv_width_ = 0.f;
	float inv_height_ = 0.f;

	static constexpr int ANGLE_TABLE_SIZE = 256;

	

public:
	ParticlePopulation(Font& debug_font)
	: hash_grid_({0, 0, world_width, world_height}),
	  renderer_(PopulationSize, debug_font),
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
		hash_grid_.clear();
		for (size_t i = 0; i < PopulationSize; ++i)
		{
			// positions are fetched and wrapped
			float& x = positions_x_[i];
			float& y = positions_y_[i];

			x = std::fmod(x + world_width, world_width);
			y = std::fmod(y + world_height, world_height);

			hash_grid_.add_object({x, y}, i);
		}
	}

	void update_particles(const bool paused = false)
	{
		// filling the left and right arrays with data
		for (size_t i = 0; i < PopulationSize; ++i)
		{
			update_angles(i, paused);
		}
	}


	void render(sf::RenderWindow& window, const bool draw_hash_grid = false, const sf::Vector2f pos = {0 ,0})
	{
		//positions_[0] = pos;
		if (draw_hash_grid)
		{
			hash_grid_.render_grid(window);
		}

		renderer_.updateParticles(positions_x_, positions_y_, neighbourhood_count_);
		renderer_.render(window);
	}


	void render_debug(sf::RenderWindow& window, const sf::Vector2f mouse_pos, const float debug_radius)
	{
		renderer_.render_debug(window, positions_x_, positions_y_, angles_, neighbourhood_count_, mouse_pos, debug_radius);
	}


private:
	inline void update_angles(const size_t index, const bool paused)
	{
		// first fetch the data we need
		float& x = positions_x_[index];
		float& y = positions_y_[index];
		float& angle = angles_[index];

		// Convert angle to lookup table index
		int angle_index = static_cast<int>((angle / two_pi) * ANGLE_TABLE_SIZE) & (ANGLE_TABLE_SIZE - 1);
		float sin_angle = sin_table_[angle_index];
		float cos_angle = cos_table_[angle_index];

		// Prefetch next particle data
		PREFETCH(&positions_x_[index + 1]);
		PREFETCH(&positions_y_[index + 1]);
		PREFETCH(&angles_[index + 1]);

		// finds the nearby indexes in each of the 9 neighbouring cells
		hash_grid_.find({x, y});

		// calculating the cumulative direction, does not need to be average to function the same
		int left = 0;
		int right = 0;

		for (int i = 0; i < hash_grid_.found_array_size; ++i)
		{
			const int other_index = hash_grid_.found_array[i];

			float to_x = positions_x_[other_index] - x;
			float to_y = positions_y_[other_index] - y;

			if (hash_grid_.at_border)
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
		const int r_minus_l = right - left;
		const auto sign = static_cast<float>(r_minus_l >= 0 ? 1 : -1);

		neighbourhood_count_[index] = right + left;

		// updating the 
		angle += (alpha + beta * (right + left) * sign) * pi_div_180;


		if (!paused)
		{
			// Update position using the look-up table
			x += gamma * cos_angle;
			y += gamma * sin_angle;
		}
	}

};
