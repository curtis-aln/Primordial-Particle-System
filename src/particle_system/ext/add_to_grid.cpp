#include "../particle_system.h"
#include <settings.h>

void ParticlePopulation::add_particles_to_grid()
{
	spatial_grid.clear();
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