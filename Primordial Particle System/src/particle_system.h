#pragma once

#include <SFML/Graphics.hpp>
#include "settings.h"
#include "utils/spatial_hash_grid.h"
#include "utils/random.h"
#include "utils/utils.h"

#include <cmath>
#include <array>

#include "utils/font.h"
#include "PPS_renderer.h"


template<size_t PopulationSize>
class ParticlePopulation : PPS_Settings
{
	// position and angle describes the particle
	std::vector<sf::Vector2f> positions_;
	std::vector<float> angles_;
	std::vector<uint16_t> neighbourhood_count_;


	// The Spatial Hash Grid Optimizes finding who is nearby
	SpatialHashGrid<hash_cells_x, hash_cells_y> hash_grid_;

	PPS_Renderer renderer_;

	// rendering and graphics
	sf::RectangleShape render_bounds_{};

	// pre-computed variables
	float inv_width_ = 0.f;
	float inv_height_ = 0.f;

	

public:
	ParticlePopulation(Font& debug_font)
	: hash_grid_({0, 0, world_width, world_height}),
	  renderer_(PopulationSize, debug_font),
	  inv_width_(1.f / world_width),
	  inv_height_(1.f / world_height)
	{

		positions_.resize(PopulationSize);
		angles_.resize(PopulationSize);
		neighbourhood_count_.resize(PopulationSize);

		// initializing particle with random initial states
		for (size_t i = 0; i < PopulationSize; ++i)
		{
			sf::FloatRect bounds = { 0, 0, world_width, world_height };
			sf::Vector2f center = { world_width / 2, world_height / 2 };
			//positions_[i] = Random::rand_pos_in_circle(center, 500);
			positions_[i] = Random::rand_pos_in_rect(bounds);
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
			sf::Vector2f& position = positions_[i];
			position.x = std::fmod(position.x + world_width, world_width);
			position.y = std::fmod(position.y + world_height, world_height);

			hash_grid_.add_object(position, i);
		}
	}

	void update_particles(const bool paused = false)
	{
		// filling the left and right arrays with data
		for (size_t i = 0; i < PopulationSize; ++i)
		{
			update_angles(i, paused);
		}

		//for (size_t i = 0; i < PopulationSize; ++i)
		//{
		//	angles_[i] += 2 * pi * decay;
		//}
	}


	void render(sf::RenderWindow& window, const bool draw_hash_grid = false, const sf::Vector2f pos = {0 ,0})
	{
		positions_[0] = pos;
		if (draw_hash_grid)
		{
			hash_grid_.render_grid(window);
		}

		renderer_.updateParticles(positions_, neighbourhood_count_);
		renderer_.render(window);

		draw_rect_outline({ 0, 0 }, {world_width, world_height}, window, 30);
	}


	void render_debug(sf::RenderWindow& window, const sf::Vector2f mouse_pos, const float debug_radius)
	{
		renderer_.render_debug(window, positions_, angles_, neighbourhood_count_, mouse_pos, debug_radius);
	}


private:
	inline void update_angles(const size_t index, const bool paused)
	{
		// first fetch the data we need
		sf::Vector2f& position = positions_[index];

		float& angle = angles_[index];

		// then pre-calculate the sin and cos values for the angle
		const float sin_angle = std::sin(angle);
		const float cos_angle = std::cos(angle);

		// finds the nearby indexes in each of the 9 neighbouring cells
		hash_grid_.find(position);

		// calculating the cumulative direction, does not need to be average to function the same
		int left = 0;
		int right = 0;

		for (int i = 0; i < hash_grid_.found_array_size; ++i)
		{
			const int other_index = hash_grid_.found_array[i];
			
			const sf::Vector2f other_position = positions_[other_index];
			sf::Vector2f direction_to = other_position - position;

			if (hash_grid_.at_border)
			{
				direction_to.x -= world_width * std::round(direction_to.x * inv_width_);
				direction_to.y -= world_height * std::round(direction_to.y * inv_height_);
			}

			const float dist_sq = direction_to.x * direction_to.x + direction_to.y * direction_to.y;

			if (dist_sq > 0 && dist_sq < visual_radius * visual_radius)
			{
				const bool is_on_right = (direction_to.x * sin_angle - direction_to.y * cos_angle) < 0;
				right += is_on_right;
				left += !is_on_right;
			}
		}

		// checking if the direction is on the right of the particle, if so converting this into -1 for false and 1 for trie
		const int r_minus_l = right - left;
		const auto sign = static_cast<float>(r_minus_l >= 0 ? 1 : -1);

		neighbourhood_count_[index] = right + left;

		// updating the angle
		angle += (alpha + beta * (right + left) * sign) * pi_div_180;

		if (paused)
		{
			return;
		}

		// updating the position
		position += {gamma* cos_angle, gamma* sin_angle};
	}

};
