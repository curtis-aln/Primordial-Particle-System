#include "../particle_system.h"
#include <cstdint>
#include <settings.h>

void ParticlePopulation::add_particles_to_grid()
{
	add_to_grid_thread_pool_.run_and_wait();
}

void ParticlePopulation::add_particle_to_grid(int i)
{
	// Wrapping the position to stay within the world
	float& x = render_data.positions_x[i];
	float& y = render_data.positions_y[i];

	if (x < 0.f || x >= world_width)  x = wrap_coord(x, world_width, inv_width_);
	if (y < 0.f || y >= world_height) y = wrap_coord(y, world_height, inv_height_);
	spatial_grid.add_object(x, y, i); // still needs to be verified thread-safe
}


void ParticlePopulation::add_particles_to_csr_grid()
{
	float* __restrict xs = render_data.positions_x.data();
	float* __restrict ys = render_data.positions_y.data();
	const uint32_t n = static_cast<uint32_t>(particle_count);

	spatial_grid.begin_build(n);            // zeroes offsets, sizes grid/sorted_x/sorted_y (replaces clear())

	// Pass 1: bound positions (in place), then count
	for (uint32_t i = 0; i < n; ++i)
	{
		float x = xs[i];
		float y = ys[i];

		if (x < 0.f || x >= world_width) { x = wrap_coord(x, world_width, inv_width_);  xs[i] = x; }
		if (y < 0.f || y >= world_height) { y = wrap_coord(y, world_height, inv_height_); ys[i] = y; }

		spatial_grid.count_object(x, y);
	}

	spatial_grid.finalize_cell_offsets();   // prefix sum + copies offsets into cell_cursor

	// Pass 2: positions are already bounded, so just scatter
	for (uint32_t i = 0; i < n; ++i)
		spatial_grid.place_object(xs[i], ys[i], i);
}