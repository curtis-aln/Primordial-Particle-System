#include "../particle_system.h"
#include <cmath>
#include <cstdint>
#include <iostream>

void ParticlePopulation::precompute_thread_ranges()
{
	const uint32_t thread_count = initial_thread_count;
	const int cells_per_thread = spatial_grid.get_total_cells() / thread_count;
	const int last_thread_cells = spatial_grid.get_total_cells() - (thread_count - 1) * cells_per_thread;


	for (int t = 0; t < (int)thread_count; ++t)
	{
		const auto [begin, end] = compute_thread_range(t, particle_count, (int)thread_count);
		updating_funcs_.emplace_back([this, t, begin, end] {
			for (int i = begin; i < end; ++i)
				update_particle_position(i); });

		add_to_grid_funcs_.emplace_back([this, t, begin, end] {
			for (int i = begin; i < end; ++i)
				add_particle_to_grid(i); });
	}

	thread_pool_.set_jobs(updating_funcs_);
	add_to_grid_thread_pool_.set_jobs(add_to_grid_funcs_);

	std::cout << "Precomputed " << updating_funcs_.size() << " particle update ranges for " << particle_count << " particles." << std::endl;
}


void ParticlePopulation::update_particle_position(int i)
{
	// Update position
	float& angle = render_data.angles_[i];
	const int angle_index = static_cast<int>((angle / two_pi) * ANGLE_TABLE_SIZE) & (ANGLE_TABLE_SIZE - 1);

	angle = fmod(angle, two_pi);
	angle += two_pi * (angle < 0.0f);

	render_data.positions_x[i] += gamma * cos_table_[angle_index];
	render_data.positions_y[i] += gamma * sin_table_[angle_index];
}

void ParticlePopulation::update_particle_positions()
{
	thread_pool_.run_and_wait();
}