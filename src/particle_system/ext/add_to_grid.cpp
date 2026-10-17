#include "../particle_system.h"
#include <cstdint>
#include <particle_system/state.h>
#include <settings.h>


void ParticlePopulation::rebuild_sorted()
{
	RenderData& in = bufs_[cur_];
	RenderData& out = bufs_[cur_ ^ 1];
	float* __restrict xs = in.positions_x.data();
	float* __restrict ys = in.positions_y.data();
	const float* __restrict as = in.angles_.data();
	float* __restrict ox = out.positions_x.data();
	float* __restrict oy = out.positions_y.data();
	float* __restrict oa = out.angles_.data();
	const uint32_t n = particle_count;

	spatial_grid.begin_build(n);
	for (uint32_t i = 0; i < n; ++i)
	{
		float x = xs[i], y = ys[i];
		if (x < 0.f || x >= world_width) { x = wrap_coord(x, world_width, inv_width_);  xs[i] = x; }
		if (y < 0.f || y >= world_height) { y = wrap_coord(y, world_height, inv_height_); ys[i] = y; }
		spatial_grid.count_object(x, y);
	}
	spatial_grid.finalize_cell_offsets();
	for (uint32_t i = 0; i < n; ++i)
	{
		const uint32_t d = spatial_grid.claim_slot(xs[i], ys[i]);
		ox[d] = xs[i]; oy[d] = ys[i]; oa[d] = as[i];
	}
	cur_ ^= 1;
	force_rebuild_ = false;
}

bool ParticlePopulation::needs_rebuild() const { return force_rebuild_ || iterations_ % add_to_grid_freq == 0; }