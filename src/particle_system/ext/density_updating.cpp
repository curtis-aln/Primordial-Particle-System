#include "../particle_system.h"


void ParticlePopulation::run_density_grid()
{
	density_grid.gaussian_smoothing_ = toggles.gaussian_smoothing_;
	density_grid.box_smoothing_ = toggles.box_smoothing_;
	density_grid.sampling_mode_ = toggles.sampling_mode_;
	density_grid.build(
		render_data.positions_x.data(),
		render_data.positions_y.data(),
		particle_count);

	solveCollisions_density();
}

void ParticlePopulation::solveCollisions_density()
{
	/*
	const uint32_t tc = thread_pool.m_thread_count;
	const int ppt = particle_count / tc;
	for (uint32_t t = 0; t < tc; ++t)
	{
		thread_pool.addTask([this, t, ppt, tc] {
			const int start = t * ppt;
			const int end = (t == tc - 1) ? particle_count : start + ppt;
			for (int i = start; i < end; ++i)
			{
				density_grid.update_particle_density(
					render_data.angles_[i],
					render_data.positions_x[i],
					render_data.positions_y[i],
					render_data.neighbourhood_count_[i],
					sin_table_, cos_table_,
					ANGLE_TABLE_SIZE);
				// Toroidal wrap (same as add_particles_to_grid)
				float& x = render_data.positions_x[i];
				float& y = render_data.positions_y[i];
				if (x < 0.f || x >= world_width)  x = wrap_coord(x, world_width, inv_width_);
				if (y < 0.f || y >= world_height) y = wrap_coord(y, world_height, inv_height_);
			}
			});
	}
	thread_pool.waitForCompletion();
	*/
}