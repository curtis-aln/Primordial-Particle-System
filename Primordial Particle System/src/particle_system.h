#pragma once

#include <SFML/Graphics.hpp>
#include "settings.h"
#include "utils/spatial_grid.h"
#include "utils/random.h"

#include <cmath>
#include <array>
#include <xmmintrin.h>

#include "PPS_renderer.h"

#include <vector>
#include <omp.h> // For OpenMP parallelization

#include "utils/thread_pool.h"


inline static constexpr float pi = 3.141592653589793238462643383279502884197f;
inline static constexpr float two_pi = 2.f * pi;
inline static constexpr float pi_div_180 = pi / 180.f;


inline float fast_round(float x) 
{
	return x >= 0.0f ? floorf(x + 0.5f) : ceilf(x - 0.5f);
}


template<size_t PopulationSize>
class ParticlePopulation : PPS_Settings
{
	// Aligned memory allocation for better vectorization
	alignas(32) std::vector<float> positions_x_;
	alignas(32) std::vector<float> positions_y_;
	alignas(32) std::vector<float> angles_;
	alignas(32) std::vector<uint16_t> neighbourhood_count_; // used by the renderer

	// Pre-computed constants for fast lookup
	static constexpr int ANGLE_TABLE_SIZE = 256;
	alignas(32) float sin_table_[ANGLE_TABLE_SIZE];
	alignas(32) float cos_table_[ANGLE_TABLE_SIZE];

	// The Spatial Grid Optimizes finding who is nearby
	SpatialGrid<grid_cells_x, grid_cells_y> spatial_grid;

	// renders the pps
	PPS_Renderer pps_renderer_;

	// pre-computed variables
	float inv_width_ = 0.f;
	float inv_height_ = 0.f;

	// temporary arrays for calculating particle interactions
	std::array<std::array<float, cell_capacity * 9>, threads> neighbour_positions_x;
	std::array<std::array<float, cell_capacity * 9>, threads> neighbour_positions_y;

	tp::ThreadPool thread_pool;

public:
	std::array<obj_idx, 200> beacons;
	int beacons_size = 0;


public:
	explicit ParticlePopulation(sf::RenderWindow& window) : spatial_grid({0, 0, world_width, world_height}),
	  pps_renderer_(PopulationSize, window), thread_pool(threads)
	{
		inv_width_ = 1.f / world_width;
		inv_height_ = 1.f / world_height;

		// resizing vectors to the population size
		positions_x_.resize(PopulationSize);
		positions_y_.resize(PopulationSize);
		angles_.resize(PopulationSize);
		neighbourhood_count_.resize(PopulationSize);

		// pre-computing values for the sin and cos tables
		for (int i = 0; i < ANGLE_TABLE_SIZE; ++i) 
		{
			float angle = (i / static_cast<float>(ANGLE_TABLE_SIZE)) * two_pi;
			sin_table_[i] = std::sin(angle);
			cos_table_[i] = std::cos(angle);
		}

		// Calculate the number of columns and rows for a nearly square grid
		int cols = static_cast<int>(std::sqrt(PopulationSize * (world_width / world_height)));
		int rows = PopulationSize / cols + (PopulationSize % cols > 0); // Ensure we cover all particles

		// Calculate the spacing between particles
		const float spacingX = world_width / cols;
		const float spacingY = world_height / rows;

		const float noise = 30.f;

		int inc = 0;
		for (int row = 0; row < rows; ++row) 
		{
			for (int col = 0; col < cols; ++col) 
			{
				if (inc < PopulationSize) 
				{
					positions_x_[inc] = col * spacingX + Random::rand11_float() * noise;
					positions_y_[inc] = row * spacingY + Random::rand11_float() * noise;
					inc++;
				}
			}
		}

		for (size_t i = 0; i < PopulationSize; ++i)
		{
			angles_[i] = Random::rand_range(0.f, 2.f * pi);
		}

		// choosing 20 random particles to put at the center
		const sf::Vector2f center = { world_width / 2.f, world_height / 2.f };
		create_cell_at(center, 25);
	}

	void create_cell_at(const sf::Vector2f position, const int particle_count)
	{
		for (int _ = 0; _ < particle_count; ++_)
		{
			const int index = Random::rand_range(size_t(0), PopulationSize);
			positions_x_[index] = position.x;
			positions_y_[index] = position.y;
		}
	}


	void add_beacons(const sf::Vector2f& position, const float radius)
	{
		beacons_size = 0;

		const float cell_size = spatial_grid.m_cellSize.x;
		const bool out_of_bounds = position.x <= cell_size || position.y <= cell_size || position.x >= world_width - cell_size || position.y >= world_height - cell_size;
		
		if (out_of_bounds)
			return;

		// getting the cell at position
		const cell_idx cell_index = spatial_grid.hash(position.x, position.y);
		const int cell_index_x = cell_index % grid_cells_x;
		const int cell_index_y = cell_index / grid_cells_x;

		// iterating over every neighbouring cell
		for (cell_idx neighbour_index_x = cell_index_x - 1; neighbour_index_x <= cell_index_x + 1; ++neighbour_index_x)
		{
			for (cell_idx neighbour_index_y = cell_index_y - 1; neighbour_index_y <= cell_index_y + 1; ++neighbour_index_y)
			{
				const cell_idx neighbour_index = neighbour_index_y * grid_cells_x + neighbour_index_x;
				const auto& neighbour_contaner = spatial_grid.grid[neighbour_index];
				const auto neighbour_size = spatial_grid.objects_count[neighbour_index];
				
				// iterating over every object per neighbour_cell
				for (auto container_index = 0; container_index < neighbour_size; ++container_index)
				{
					const obj_idx index = neighbour_contaner[container_index];
					const sf::Vector2f particle_pos = { positions_x_[index], positions_y_[index] };
					const sf::Vector2f dir = particle_pos - position;
					const float dist = dir.x * dir.x + dir.y * dir.y;

					if (dist < radius * radius)
					{
						beacons[beacons_size++] = index;
					}
					
					if (beacons_size >= beacons.size())
					{
						return;
					}
				}
			}
		}

	}


	void render_beacons(sf::RenderWindow& window)
	{
		const float rad = PPS_Graphics::particle_radius;

		sf::CircleShape beacon_body;
		beacon_body.setFillColor({ 255, 255, 255 });
		beacon_body.setRadius(rad);

		for (int beacon_container_index = 0; beacon_container_index < beacons_size; ++beacon_container_index)
		{
			int beacon_index = beacons[beacon_container_index];

			const sf::Vector2f position = { positions_x_[beacon_index] - rad, positions_y_[beacon_index] - rad };
			beacon_body.setPosition(position);
			window.draw(beacon_body);
		}
	}


	// At the start of every iteration. all the particles need to be removed from the grid and re-added
	void add_particles_to_grid()
	{
		spatial_grid.clear();

		const uint32_t thread_count = thread_pool.m_thread_count;
		const size_t particles_per_thread = PopulationSize / thread_count;
		const size_t last_thread_particles = PopulationSize - (thread_count - 1) * particles_per_thread;

		for (uint32_t t = 0; t < thread_count; ++t)
		{
			thread_pool.addTask([this, t, particles_per_thread, last_thread_particles, thread_count] {
				const size_t start = t * particles_per_thread;
				const size_t end = (t == thread_count - 1) ? start + last_thread_particles : start + particles_per_thread;

				for (size_t i = start; i < end; ++i)
				{
					// positions are fetched and wrapped
					float& x = positions_x_[i];
					float& y = positions_y_[i];

					const bool at_border_x = x < 0.0f || x >= world_width;
					const bool at_border_y = y < 0.0f || y >= world_height;

					if (at_border_x)
					{
						x -= world_width * std::floor(x * inv_width_);
					}

					if (at_border_y)
					{
						y -= world_height * std::floor(y * inv_height_);
					}

					spatial_grid.add_object(x, y, i);
				}
				});
		}

		thread_pool.waitForCompletion();
	}


	void update_particles(const bool paused = false)
	{
		solveCollisions();

		if (!paused)
		{
			update_particle_positions();
		}

	}


	void render(sf::RenderWindow& window, const sf::Vector2f display_top_left, const sf::Vector2f display_bottom_right, const bool draw_spatial_grid = false, const sf::Vector2f pos = {0 ,0})
	{
		//positions_[0] = pos;
		if (draw_spatial_grid)
		{
			spatial_grid.render_grid(window);
		}

		pps_renderer_.render_particles(positions_x_, positions_y_, neighbourhood_count_);
	}


	void render_debug(sf::RenderWindow& window, const sf::Vector2f mouse_pos, const float debug_radius)
	{
		pps_renderer_.render_debug(positions_x_, positions_y_, angles_, neighbourhood_count_, mouse_pos, debug_radius);
	}




private:
	void update_particle_positions()
	{
		const uint32_t thread_count = thread_pool.m_thread_count;
		const int particles_per_thread = particle_count / thread_count;
		const int last_thread_particles = particle_count - (thread_count - 1) * particles_per_thread;

		for (uint32_t t = 0; t < thread_count; ++t)
		{
			thread_pool.addTask([this, t, particles_per_thread, last_thread_particles, thread_count] {
				const int start = t * particles_per_thread;
				const int end = (t == thread_count - 1) ? start + last_thread_particles : start + particles_per_thread;

				for (int i = start; i < end; ++i)
				{
					// Update position
					float& angle = angles_[i];
					const int angle_index = static_cast<int>((angle / two_pi) * ANGLE_TABLE_SIZE) & (ANGLE_TABLE_SIZE - 1);

					angle = fmod(angle, two_pi);
					angle += two_pi * (angle < 0.0f);

					positions_x_[i] += gamma * cos_table_[angle_index];
					positions_y_[i] += gamma * sin_table_[angle_index];
				}
				});
		}

		thread_pool.waitForCompletion();
	}

	void solveCollisionThreaded(uint32_t start, uint32_t end, int thread_idx)
	{
		for (uint32_t idx{ start }; idx < end; ++idx) 
		{
			process_cell(idx, neighbour_positions_x[thread_idx], neighbour_positions_y[thread_idx]);
		}
	}


	void solveCollisions()
	{
		// Multi-thread grid
		const uint32_t thread_count = thread_pool.m_thread_count;
		const uint32_t slice_size = (grid_cells_x * grid_cells_y) / thread_count;
		const uint32_t last_cell = thread_count * slice_size;

		// Collision pass
		for (uint32_t i = 0; i < thread_count; ++i)
		{
			thread_pool.addTask([this, i, slice_size] 
			{
				uint32_t const start = i * slice_size;
				uint32_t const end = start + slice_size;
				solveCollisionThreaded(start, end, i);
			});
		}

		// process rest if the world is not divisible by the thread count
		if (last_cell < grid_cells_x * grid_cells_y)
		{
			thread_pool.addTask([this, last_cell]
			{
				solveCollisionThreaded(last_cell, grid_cells_x * grid_cells_y, 0);
			});
		}

		thread_pool.waitForCompletion();
	}



	void process_cell(
		const cell_idx cell_index,
		std::array<float, cell_capacity * 9>& n_positions_x,
		std::array<float, cell_capacity * 9>& n_positions_y)
	{
		// for a given cell this function will access its particle contents. and for each one of them it will update them based off the information from the
		// neighbouring 9 cells.
		int neighbours_size = 0;

		const int cell_index_x = cell_index % grid_cells_x;
		const int cell_index_y = cell_index / grid_cells_x;
		const bool at_border = cell_index_x == 0 || cell_index_y == 0 || cell_index_x == grid_cells_x - 1 || cell_index_y == grid_cells_y - 1;

		process_neighbouring_cell(n_positions_x, n_positions_y, neighbours_size, cell_index_x - 1, cell_index_y - 1, true, true);
		process_neighbouring_cell(n_positions_x, n_positions_y, neighbours_size, cell_index_x    , cell_index_y - 1, false, true);
		process_neighbouring_cell(n_positions_x, n_positions_y, neighbours_size, cell_index_x + 1, cell_index_y - 1, true, true);
		process_neighbouring_cell(n_positions_x, n_positions_y, neighbours_size, cell_index_x - 1, cell_index_y    , true, false);
		process_neighbouring_cell(n_positions_x, n_positions_y, neighbours_size, cell_index_x    , cell_index_y    , false, false);
		process_neighbouring_cell(n_positions_x, n_positions_y, neighbours_size, cell_index_x + 1, cell_index_y    , true, false);
		process_neighbouring_cell(n_positions_x, n_positions_y, neighbours_size, cell_index_x - 1, cell_index_y + 1, true, true);
		process_neighbouring_cell(n_positions_x, n_positions_y, neighbours_size, cell_index_x    , cell_index_y + 1, false, true);
		process_neighbouring_cell(n_positions_x, n_positions_y, neighbours_size, cell_index_x + 1, cell_index_y + 1, true, true);

		// updating the particles
		const auto& cell_contents = spatial_grid.grid[cell_index];
		const uint8_t cell_size = spatial_grid.objects_count[cell_index];

#pragma omp parallel for
		for (uint8_t idx = 0; idx < cell_size; ++idx)
		{
			update_particle(cell_contents[idx], at_border, n_positions_x, n_positions_y, neighbours_size);
		}

	}

	inline void process_neighbouring_cell(
		std::array<float, cell_capacity * 9>& n_positions_x,
		std::array<float, cell_capacity * 9>& n_positions_y,
		int& neighbours_size,
		int32_t neighbour_index_x, int32_t neighbour_index_y,
		bool check_x = true,
		bool check_y = true)
	{
		// Fast modulo for positive and negative numbers
		if (check_x)
		{
			neighbour_index_x = neighbour_index_x >= 0 ?
				(neighbour_index_x < grid_cells_x ? neighbour_index_x : neighbour_index_x - grid_cells_x) :
				(neighbour_index_x + grid_cells_x);
		}

		if (check_y)
		{
			neighbour_index_y = neighbour_index_y >= 0 ?
				(neighbour_index_y < grid_cells_y ? neighbour_index_y : neighbour_index_y - grid_cells_y) :
				(neighbour_index_y + grid_cells_y);
		}

		// processing the neighbour
		const uint32_t neighbour_index = neighbour_index_y * grid_cells_x + neighbour_index_x;
		const auto& contents = spatial_grid.grid[neighbour_index];
		const auto size = spatial_grid.objects_count[neighbour_index];

		// adding the neighbour data to the array
#pragma omp parallel for
		for (uint8_t idx = 0; idx < size; ++idx)
		{
			const obj_idx object_index = contents[idx];
			n_positions_x[neighbours_size] = positions_x_[object_index];
			n_positions_y[neighbours_size] = positions_y_[object_index];
			++neighbours_size;
		}
	}


	inline void update_particle(const obj_idx index, const bool at_border,
		std::array<float, cell_capacity * 9>& n_positions_x,
		std::array<float, cell_capacity * 9>& n_positions_y,
		const int neighbours_size)
	{
		// first fetch the data we need
		float& x = positions_x_[index];
		float& y = positions_y_[index];
		float& angle = angles_[index];

		// Convert angle to lookup table index
		const int angle_index = static_cast<int>((angle / two_pi) * ANGLE_TABLE_SIZE) & (ANGLE_TABLE_SIZE - 1);
		const float sin_angle = sin_table_[angle_index];
		const float cos_angle = cos_table_[angle_index];

		// calculating the total and right particle count
		int total = 0;
		int right = 0;

		float wrap_factor_x = at_border ? world_width : 0.0f; // todo use at_border_x and at_border_y + if conditions
		float wrap_factor_y = at_border ? world_height : 0.0f;

		for (uint32_t i{ 0 }; i < neighbours_size; ++i)
		{
			float to_x = n_positions_x[i] - x;
			float to_y = n_positions_y[i] - y;

			to_x -= wrap_factor_x * fast_round(to_x * inv_width_);
			to_y -= wrap_factor_y * fast_round(to_y * inv_height_);

			const float dist_sq = to_x * to_x + to_y * to_y;

			if (dist_sq > 0 && dist_sq < visual_radius * visual_radius)
			{
				right += (to_x * sin_angle - to_y * cos_angle) < 0;
				++total;
			}
		}

		// checking if the direction is on the right of the particle, if so converting this into -1 for false and 1 for trie
		const int left = total - right;
		const auto sign = static_cast<float>(((right - left) >= 0) * 2 - 1);
		neighbourhood_count_[index] = right + left;

		angle += (alpha + beta * (right + left) * sign) * pi_div_180;
	}
};
