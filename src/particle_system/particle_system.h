#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <array>
#include <xmmintrin.h>
#include <numbers>
#include <vector>
#include <omp.h> // For OpenMP parallelization

#include "PPS_renderer.h"
#include "beacons.h"
#include "state.h"

#include "../settings.h"

#include "../utils/spatial_grid/simple_spatial_grid.h"
#include "../utils/random.h"
#include "../utils/thread_pool.h"
#include "utils/smooth_frame_rates.h"


// pre-computing constants
inline static constexpr float pi = std::numbers::pi_v<float>;
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
public:
	// Aligned memory allocation for better vectorization
	RenderData render_data{};
	WorldToggles toggles{};

private:

	// Pre-computed constants for fast lookup when calculating the direction particles must go
	static constexpr int ANGLE_TABLE_SIZE = 256;
	alignas(64) float sin_table_[ANGLE_TABLE_SIZE];
	alignas(64) float cos_table_[ANGLE_TABLE_SIZE];

	// The Spatial Grid Optimizes finding what particles are nearby
	SimpleSpatialGrid spatial_grid;

	// pre-computed values for wrapping the particles in the world
	float inv_width_ = 0.f;
	float inv_height_ = 0.f;

	// Multi-threading adding to the spatial grid and updating neighbourhood positions
	tp::ThreadPool thread_pool;

	struct CellRange { uint32_t start, end; };
	std::vector<CellRange> collision_ranges_; // computed once, reused every frame
	std::vector<CellRange> grid_insert_ranges_;

	// Statistics
	FrameRateSmoothing<100> frame_rate_smoothing_{};

public:
	Beacons<max_beacon_count, grid_cells_x, grid_cells_y> beacons{ &spatial_grid, 
		& render_data.positions_x, & render_data.positions_y, spatial_grid.cell_width, world_width, world_height };
	
	

	void fill_snapshot(SimSnapshot& snapshot);
	


public:
	explicit ParticlePopulation();


	void init_grid_positioning();
	void create_cell_at(const sf::Vector2f position, const int particle_count);
	
	void add_particles_to_grid();

	void update_particles();


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
		const std::array<float, cell_capacity * 9>& n_positions_x,
		const std::array<float, cell_capacity * 9>& n_positions_y,
		const int neighbours_size);


	void precompute_thread_ranges();
	
};
