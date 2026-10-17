#include "../particle_system.h"
#include <algorithm>
#include <cstdint>
#include <settings.h>
#include <utils/css_spatial_grid/css_spatial_grid.h>


namespace
{
	constexpr float radius_sq = PPS_Settings::visual_radius * PPS_Settings::visual_radius;

	// Counts one neighbour if it is inside the vision radius (and is not the observer itself, d_sq == 0).
	// dx, dy = vector from observer to neighbour.
	inline void tally_if_visible(const float dx, const float dy, const Observer& self, NeighbourTally& tally)
	{
		const float dist_sq = dx * dx + dy * dy;
		if (dist_sq > 0.f & dist_sq < radius_sq)   // bitwise & avoids a branch
		{
			// Cross product with the heading vector: negative = neighbour is on the observer's right.
			tally.on_right += (dx * self.sin_heading - dy * self.cos_heading) < 0.f;
			++tally.total;
		}
	}
}


void ParticlePopulation::precompute_cell_update_ranges()
{
	collision_funcs_.clear();
	for (uint32_t t = 0; t < initial_thread_count; ++t)
		collision_funcs_.emplace_back([this] {
		constexpr uint32_t rows_per_chunk = 4;
		const uint32_t rows = grid_cells_y;
		const uint32_t chunks = (rows + rows_per_chunk - 1) / rows_per_chunk;
		for (uint32_t c; (c = next_row_chunk_.fetch_add(1, std::memory_order_relaxed)) < chunks; )
		{
			const uint32_t begin = c * rows_per_chunk * grid_cells_x;
			const uint32_t end = std::min((c + 1) * rows_per_chunk, rows) * grid_cells_x;
			for (uint32_t cell = begin; cell < end; ++cell)
				process_cell(cell);
		}
			});
	collision_thread_pool_.set_jobs(collision_funcs_);
}

void ParticlePopulation::solve_neighbourhood_detection()
{
	next_row_chunk_.store(0, std::memory_order_relaxed);
	collision_thread_pool_.run_and_wait();
}


GridView ParticlePopulation::make_grid_view() const
{
	return { spatial_grid.grid_cell_offsets.data(),
			 bufs_[cur_].positions_x.data(), bufs_[cur_].positions_y.data() };
}

// Border cells have neighbours that wrap around the world, so they need the slow path.
bool ParticlePopulation::is_border_cell(const int cell_x, const int cell_y) const
{
	return cell_x == 0 || cell_x == grid_cells_x - 1
		|| cell_y == 0 || cell_y == grid_cells_y - 1;
}

// Looks up sin/cos of the heading from the lookup tables.
Observer ParticlePopulation::make_observer(const float x, const float y, const float heading) const
{
	const int table_index = static_cast<int>(heading * angle_to_index) & (ANGLE_TABLE_SIZE - 1);
	return { x, y, sin_table_[table_index], cos_table_[table_index] };
}

__forceinline void ParticlePopulation::scan_slots(const GridView& grid, const uint32_t first_slot, const uint32_t end_slot,
	const Observer& self, NeighbourTally& tally) const
{
	const float* __restrict gx = grid.x;
	const float* __restrict gy = grid.y;
	int total = 0, on_right = 0;                       // locals stay in registers

	for (uint32_t slot = first_slot; slot < end_slot; ++slot)
	{
		const float dx = gx[slot] - self.x;
		const float dy = gy[slot] - self.y;
		const float d_sq = dx * dx + dy * dy;
		const bool visible = (d_sq > 0.f) & (d_sq < radius_sq);
		total += visible;
		on_right += visible & ((dx * self.sin_heading - dy * self.cos_heading) < 0.f);
	}
	tally.total += total;
	tally.on_right += on_right;
}

// Same, with toroidal wrapping of dx/dy (border cells only, so keep it simple)
__forceinline void ParticlePopulation::scan_slots_wrapped(const GridView& grid, const uint32_t first_slot, const uint32_t end_slot,
	const Observer& self, NeighbourTally& tally) const
{
	const float* __restrict gx = grid.x;
	const float* __restrict gy = grid.y;
	int total = 0, on_right = 0;

	for (uint32_t slot = first_slot; slot < end_slot; ++slot)
	{
		float dx = gx[slot] - self.x;
		float dy = gy[slot] - self.y;
		dx -= world_width * fast_round(dx * inv_width_);
		dy -= world_height * fast_round(dy * inv_height_);
		const float d_sq = dx * dx + dy * dy;
		const bool visible = (d_sq > 0.f) & (d_sq < radius_sq);
		total += visible;
		on_right += visible & ((dx * self.sin_heading - dy * self.cos_heading) < 0.f);
	}
	tally.total += total;
	tally.on_right += on_right;
}


#include <immintrin.h>
#include <emmintrin.h>
#include <cmath>
#include <particle_system/state.h>

#if defined(__AVX2__)
namespace {
	inline int hsum_epi32(__m256i v)
	{
		__m128i s = _mm_add_epi32(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1));
		s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(1, 0, 3, 2)));
		s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 3, 0, 1)));
		return _mm_cvtsi128_si32(s);
	}

	struct SimdObserver { __m256 x, y, sn, cs; };
	struct SimdTally { __m256i total = _mm256_setzero_si256(), on_right = _mm256_setzero_si256(); };

	__forceinline void scan_range_avx2(const GridView& g, uint32_t first, uint32_t end,
		const SimdObserver& o, SimdTally& acc)
	{
		const __m256  r2 = _mm256_set1_ps(radius_sq);
		const __m256  zero = _mm256_setzero_ps();
		const __m256i iota = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);

		for (uint32_t s = first; s < end; s += 8)
		{
			const __m256 dx = _mm256_sub_ps(_mm256_loadu_ps(g.x + s), o.x);
			const __m256 dy = _mm256_sub_ps(_mm256_loadu_ps(g.y + s), o.y);
			const __m256 d2 = _mm256_add_ps(_mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy));

			__m256 vis = _mm256_and_ps(_mm256_cmp_ps(d2, zero, _CMP_GT_OQ),
				_mm256_cmp_ps(d2, r2, _CMP_LT_OQ));
			// mask lanes beyond end_slot (tail)
			vis = _mm256_and_ps(vis, _mm256_castsi256_ps(
				_mm256_cmpgt_epi32(_mm256_set1_epi32(int(end - s)), iota)));

			const __m256 cross = _mm256_sub_ps(_mm256_mul_ps(dx, o.sn), _mm256_mul_ps(dy, o.cs));
			const __m256 right = _mm256_and_ps(vis, _mm256_cmp_ps(cross, zero, _CMP_LT_OQ));

			acc.total = _mm256_sub_epi32(acc.total, _mm256_castps_si256(vis));    // mask = -1 -> +1
			acc.on_right = _mm256_sub_epi32(acc.on_right, _mm256_castps_si256(right));
		}
	}
}

NeighbourTally ParticlePopulation::tally_interior_neighbourhood(const GridView& grid, const cell_idx cell, const Observer& self) const
{
	const uint32_t* cs = grid.cell_start + cell;
	const int rs = grid_cells_x;
	const SimdObserver o{ _mm256_set1_ps(self.x), _mm256_set1_ps(self.y),
						  _mm256_set1_ps(self.sin_heading), _mm256_set1_ps(self.cos_heading) };
	SimdTally acc;
	scan_range_avx2(grid, cs[-rs - 1], cs[-rs + 2], o, acc);
	scan_range_avx2(grid, cs[-1], cs[2], o, acc);
	scan_range_avx2(grid, cs[rs - 1], cs[rs + 2], o, acc);
	return { hsum_epi32(acc.total), hsum_epi32(acc.on_right) };
}
#else
// ...existing scalar tally_interior_neighbourhood...
#endif
// Border cell: visit each of the 9 neighbour cells with wrapped cell coordinates and wrapped distances.
NeighbourTally ParticlePopulation::tally_border_neighbourhood(const GridView& grid, const int cell_x, const int cell_y, const Observer& self) const
{
	NeighbourTally tally;
	for (int offset_y = -1; offset_y <= 1; ++offset_y)
		for (int offset_x = -1; offset_x <= 1; ++offset_x)
		{
			const int neighbour_x = (cell_x + offset_x + grid_cells_x) % grid_cells_x;
			const int neighbour_y = (cell_y + offset_y + grid_cells_y) % grid_cells_y;
			const int neighbour_cell = neighbour_y * grid_cells_x + neighbour_x;
			scan_slots_wrapped(grid, grid.cell_start[neighbour_cell], grid.cell_start[neighbour_cell + 1], self, tally);
		}
	return tally;
}

// Turns the heading based on how many neighbours are on each side.
void ParticlePopulation::steer(float& heading, const NeighbourTally& tally) const
{
	const int   left_count = tally.total - tally.on_right;
	const float turn_direction = static_cast<float>((tally.on_right > left_count) - (tally.on_right < left_count)); // +1, 0 or -1
	heading += (alpha + beta * static_cast<float>(tally.total) * turn_direction) * pi_div_180;
}

// Updates every particle in one cell from its 3x3 neighbourhood.
void ParticlePopulation::process_cell(const cell_idx cell)
{
	const GridView grid = make_grid_view();
	const uint32_t first = grid.cell_start[cell], end = grid.cell_start[cell + 1];
	if (first == end) return;

	const RenderData& in = bufs_[cur_];
	RenderData& out = bufs_[cur_ ^ 1];
	const int  cell_x = cell % grid_cells_x, cell_y = cell / grid_cells_x;
	const bool is_border = is_border_cell(cell_x, cell_y);

	for (uint32_t s = first; s < end; ++s)
	{
		float heading = in.angles_[s];
		const Observer self = make_observer(grid.x[s], grid.y[s], heading);
		const NeighbourTally tally = !is_border ? tally_interior_neighbourhood(grid, cell, self)
			: tally_border_neighbourhood(grid, cell_x, cell_y, self);
		out.neighbourhood_count_[s] = static_cast<uint16_t>(tally.total);
		steer(heading, tally);

		heading -= two_pi * std::floor(heading * inv_two_pi_);
		const int idx = static_cast<int>(heading * angle_to_index) & (ANGLE_TABLE_SIZE - 1);
		const float c = cos_table_[idx], sn = sin_table_[idx];
		out.angles_[s] = heading;
		out.positions_x[s] = grid.x[s] + gamma * c;
		out.positions_y[s] = grid.y[s] + gamma * sn;
		out.cos_angles_[s] = c;
		out.sin_angles_[s] = sn;
	}
}
