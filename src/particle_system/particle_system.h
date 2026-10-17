#pragma once

#include <cmath>
#include <numbers>
#include <vector>
#include <xmmintrin.h>

#include "state.h"

#include "../settings.h"

#include "../utils/css_spatial_grid/css_spatial_grid.h"
#include <functional>
#include <functional>
#include <functional>
#include <SFML/System/Vector2.hpp>
#include <simulation/context/sim_snapshot.h>
#include <utils/custom_clock.h>

#include "../utils/thread_pool.h"
#include "density_grid/density_grid.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <utility>


// pre-computing constants
inline static constexpr float pi = std::numbers::pi_v<float>;
inline static constexpr float two_pi = 2.f * pi;
inline static constexpr float pi_div_180 = pi / 180.f;


// a faster implementation of the round function
inline float fast_round(float x)
{
	return x >= 0.0f ? floorf(x + 0.5f) : ceilf(x - 0.5f);
}

// Splits `total` items evenly across `thread_count` threads and returns the
// [begin, end) range owned by `thread_index`. Returns {0, 0} if there's nothing to do.
inline static std::pair<int, int> compute_thread_range(int thread_index, int total, int thread_count)
{
	if (total == 0)
		return { 0, 0 };

	const int chunk_size = std::max(1, (total + thread_count - 1) / thread_count);
	const int begin = std::min(thread_index * chunk_size, total);
	const int end = std::min(begin + chunk_size, total);

	return { begin, end };
}

// Read-only view of the CSR grid for one query phase.
struct GridView
{
	const uint32_t* cell_start;   // size cells+1; cell c = slots [cell_start[c], cell_start[c+1])
	const float* x;            // sorted_x, indexed by slot
	const float* y;            // sorted_y, indexed by slot
};

// The particle whose neighbourhood is being measured.
struct Observer { float x, y, sin_heading, cos_heading; };

// Result of scanning a neighbourhood.
struct NeighbourTally { int total = 0; int on_right = 0; };


class ParticlePopulation : PPS_Settings
{
public:
	// Aligned memory allocation for better vectorization
	WorldToggles toggles{};

	// Iterations per second can be different to frames per second as we sometimes run several iterations per frame to speed up the simulation
	CustomClock<100> iterations_clock_{};

	// The Spatial Grid Optimizes finding what particles are nearby
	CSRSpatialGrid spatial_grid;

private:
	// Pre-computed constants for fast lookup when calculating the direction particles must go
	static constexpr int ANGLE_TABLE_SIZE = 1024;
	alignas(64) float sin_table_[ANGLE_TABLE_SIZE];
	alignas(64) float cos_table_[ANGLE_TABLE_SIZE];

	RenderData bufs_[2];          // replaces render_data; positions_x/y + angles_ are double-buffered
	int  cur_ = 0;
	bool force_rebuild_ = true;   // set in init_grid_positioning / create_cell_at / randomize_*
	static constexpr float inv_two_pi_ = 1.f / two_pi;
	static constexpr float angle_to_index = ANGLE_TABLE_SIZE / two_pi;


	DensityGrid density_grid{ world_width, world_height, visual_radius / 4.f };

	// pre-computed values for wrapping the particles in the world
	float inv_width_ = 0.f;
	float inv_height_ = 0.f;

	// Multi-threading adding to the spatial grid and updating neighbourhood positions
	std::vector<std::function<void()>> collision_funcs_;
	BarrierThreadPool collision_thread_pool_{ initial_thread_count };

	std::atomic<uint32_t> next_row_chunk_{ 0 };


public:
	DensitySettings& density_settings() { return density_grid; }

	size_t iterations_ = 0;

	void fill_snapshot(SimSnapshot& snapshot);

	void rebuild_sorted();

	bool needs_rebuild() const;


public:
	explicit ParticlePopulation();

	// User Set functions
	void update_particles();

	void randomize_sim();
	void set_thread_count(int threads);

	void init_grid_positioning();
	void create_cell_at(const sf::Vector2f position, const int particle_count);

private:


	void add_particle_to_grid(int i);

	// Density grid functions
	void run_density_grid();
	void solveCollisions_density();

	// CSR resolution functions
	void           process_cell(cell_idx cell);
	GridView       make_grid_view() const;
	bool           is_border_cell(int cell_x, int cell_y) const;
	Observer       make_observer(float x, float y, float heading) const;
	NeighbourTally tally_interior_neighbourhood(const GridView& grid, cell_idx cell, const Observer& self) const;
	NeighbourTally tally_border_neighbourhood(const GridView& grid, int cell_x, int cell_y, const Observer& self) const;
	void           scan_slots(const GridView& grid, uint32_t first_slot, uint32_t end_slot, const Observer& self, NeighbourTally& tally) const;
	void           scan_slots_wrapped(const GridView& grid, uint32_t first_slot, uint32_t end_slot, const Observer& self, NeighbourTally& tally) const;
	void           steer(float& heading, const NeighbourTally& tally) const;


	// init functions
	void init_sin_cos_tables();
	void init_particle_vectors();
	void precompute_cell_update_ranges();

	void randomize_angles();

	// Updating 
	void update_particle_positions();

	void solve_neighbourhood_detection();

	void update_particle_position(int i);

};
