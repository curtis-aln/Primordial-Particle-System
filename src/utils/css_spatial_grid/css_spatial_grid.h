#pragma once

#include "../spatial_grid/fixed_span.h"
#include <algorithm>
#include <cstdint>
#include <vector>

// Todo
// - run a big simulation to see if vectors can be stored in stack memory

using cell_idx = uint32_t;
using obj_idx = uint32_t;

struct CSRSpatialGrid
{
	uint32_t CellsX = 0;
	uint32_t CellsY = 0;

	float cell_width = 0;
	float cell_height = 0;
	float world_width = 0;
	float world_height = 0;

	float inv_cell_w = 0, inv_cell_h = 0;

	alignas(64) std::vector<uint32_t> cell_cursor{};   // size cells, scatter scratch
	alignas(64) std::vector<float>    sorted_x{}, sorted_y{};

	/// Particle ids sorted by cell. Cell c occupies [grid_cell_offsets[c], grid_cell_offsets[c+1]).
	alignas(64) std::vector<obj_idx>  grid{};             // flat 1D: [cell * capacity + slot]

	/// Each cell has a dynamic capacity, with no global max capacity
	/// grid_cell_offsets contains the offset of each cell in the grid array, so we can find the start of each cell's data.
	/// To correctly fill this index, fill it cell by cell instead of particle by particle.	
	/// Size cells+1. Last entry = total particle count (sentinel).
	alignas(64) std::vector<uint32_t>  grid_cell_offsets{};
	uint32_t population_size_ = 0;

public:
	// Returns the number of objects in a given cell via subtracting the offsets of the next cell from the current cell.
	uint32_t get_cell_size(const cell_idx c) const
	{
		return grid_cell_offsets[c + 1] - grid_cell_offsets[c];
	}

	void begin_build(uint32_t n)
	{
		std::fill(grid_cell_offsets.begin(), grid_cell_offsets.end(), 0u);

		//padding the sorted arrays so the last vector can over read
		grid.resize(n); sorted_x.resize(n + 8, 0.f); sorted_y.resize(n + 8, 0.f);
	}

	// phase 1 (your add_cell_slot, renamed): counts live at off[c+1]
	void count_object(float x, float y) { ++grid_cell_offsets[hash(x, y) + 1]; }

	// phase 2: your prefix sum is correct; add the cursor copy
	void finalize_cell_offsets()
	{
		for (size_t i = 1; i < grid_cell_offsets.size(); ++i)
			grid_cell_offsets[i] += grid_cell_offsets[i - 1];
		std::copy_n(grid_cell_offsets.begin(), cell_cursor.size(), cell_cursor.begin());
	}

	// phase 3: claim a slot and write the particle into it
	void place_object(float x, float y, obj_idx id)
	{
		const uint32_t dst = cell_cursor[hash(x, y)]++;
		grid[dst] = id;
		sorted_x[dst] = x;
		sorted_y[dst] = y;
	}

	void build(const float* x, const float* y, uint32_t n)
	{
		begin_build(n);

		for (uint32_t i = 0; i < n; ++i)
			count_object(x[i], y[i]);

		finalize_cell_offsets();

		for (uint32_t i = 0; i < n; ++i)
			place_object(x[i], y[i], i);
	}

public:
	explicit CSRSpatialGrid(uint32_t cells_x, uint32_t cells_y,
		float world_width, float world_height)
		: CellsX(cells_x), CellsY(cells_y),
		world_width(world_width), world_height(world_height)
	{
		const uint32_t total = CellsX * CellsY;
		grid_cell_offsets.resize(total + 1, 0);

		cell_cursor.resize(total);

		cell_width = world_width / static_cast<float>(CellsX);
		cell_height = world_height / static_cast<float>(CellsY);

		inv_cell_w = 1.f / cell_width;
		inv_cell_h = 1.f / cell_height;
	}

	~CSRSpatialGrid() = default;


	cell_idx inline hash(const float x, const float y) const
	{
		const uint32_t cx = std::min(static_cast<uint32_t>(std::max(x, 0.f) * inv_cell_w), CellsX - 1);
		const uint32_t cy = std::min(static_cast<uint32_t>(std::max(y, 0.f) * inv_cell_h), CellsY - 1);
		return cy * CellsX + cx;
	}

	cell_idx inline add_object(const float x, const float y, const size_t obj_id)
	{
		return 0;
	}

	size_t get_total_cells() const { return CellsX * CellsY; }

	void find(const float x, const float y, FixedSpan<obj_idx>* container) const
	{
		find_from_index(hash(x, y), container);
	}

	void find_from_index(const cell_idx index, FixedSpan<obj_idx>* container) const
	{

	}

	void refresh_positions(const float* __restrict xs, const float* __restrict ys)
	{
		const uint32_t n = static_cast<uint32_t>(grid.size());
		for (uint32_t slot = 0; slot < n; ++slot)
		{
			const obj_idx id = grid[slot];
			sorted_x[slot] = xs[id];
			sorted_y[slot] = ys[id];
		}
	}

	// This converts a world-space rect into a clamped grid cell range with zero allocation
	void get_cell_range(float wx0, float wy0, float wx1, float wy1,
		uint32_t& cx0, uint32_t& cy0,
		uint32_t& cx1, uint32_t& cy1) const
	{
		cx0 = static_cast<uint32_t>(std::clamp(wx0 / cell_width, 0.f, static_cast<float>(CellsX - 1)));
		cy0 = static_cast<uint32_t>(std::clamp(wy0 / cell_height, 0.f, static_cast<float>(CellsY - 1)));
		cx1 = static_cast<uint32_t>(std::clamp(wx1 / cell_width, 0.f, static_cast<float>(CellsX - 1)));
		cy1 = static_cast<uint32_t>(std::clamp(wy1 / cell_height, 0.f, static_cast<float>(CellsY - 1)));
	}
};