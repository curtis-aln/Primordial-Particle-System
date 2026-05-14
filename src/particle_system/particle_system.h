#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <array>
#include <xmmintrin.h>
#include <vector>
#include <omp.h> // For OpenMP parallelization

#include "PPS_renderer.h"
#include "beacons.h"

#include "../settings.h"

#include "../utils/spatial_grid/simple_spatial_grid.h"
#include "../utils/random.h"
#include "../utils/thread_pool.h"


// pre-computing constants
inline static constexpr float pi = 3.1415f;
inline static constexpr float two_pi = 2.f * pi;
inline static constexpr float pi_div_180 = pi / 180.f;


// a faster implementation of the round function
inline float fast_round(float x) 
{
	return x >= 0.0f ? floorf(x + 0.5f) : ceilf(x - 0.5f);
}

// Temporary arrays for calculating particle interactions. One array needed for each thread to avoid issues with data writing.
using TL_NeighbourPositions = std::array<float, PPS_Settings::cell_capacity * 9>;
extern thread_local TL_NeighbourPositions neighbour_positions_x;
extern thread_local TL_NeighbourPositions neighbour_positions_y;


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
	SimpleSpatialGrid spatial_grid;

	// pre-computed
	float inv_width_ = 0.f;
	float inv_height_ = 0.f;

	tp::ThreadPool thread_pool;

	struct CellRange { uint32_t start, end; };
	std::vector<CellRange> collision_ranges_; // computed once, reused every frame
	std::vector<CellRange> grid_insert_ranges_;

public:
	Beacons<max_beacon_count, grid_cells_x, grid_cells_y> beacons{ &spatial_grid, 
		&positions_x_, &positions_y_, spatial_grid.cell_width, world_width, world_height };

	PPS_Renderer pps_renderer_;


public:
	explicit ParticlePopulation(sf::RenderWindow& window);


	void init_grid_positioning();
	void create_cell_at(const sf::Vector2f position, const int particle_count);
	
	void add_particles_to_grid();

	void update_particles(const bool paused = false);
	void render(sf::RenderWindow& window, const bool draw_spatial_grid = false, const sf::Vector2f pos = { 0 ,0 });
	void render_debug(sf::RenderWindow& window, const sf::Vector2f mouse_pos, const float debug_radius);



private:
	void init_sin_cos_tables();
	void init_particle_vectors();

	void randomize_angles();

	void update_particle_positions();

	void solveCollisions();

	void process_cell(
		const cell_idx cell_index,
		std::array<float, cell_capacity * 9>& n_positions_x,
		std::array<float, cell_capacity * 9>& n_positions_y);


	inline void add_neighbour_cells_particles(
		std::array<float, cell_capacity * 9>& n_positions_x,
		std::array<float, cell_capacity * 9>& n_positions_y,
		int& neighbours_size,
		int32_t neighbour_index_x, int32_t neighbour_index_y,
		bool check_x = true,
		bool check_y = true);

	inline void update_particle(const obj_idx index, const bool at_border_x, const bool at_border_y,
		std::array<float, cell_capacity * 9>& n_positions_x,
		std::array<float, cell_capacity * 9>& n_positions_y,
		const int neighbours_size);


	void precompute_thread_ranges();
};
