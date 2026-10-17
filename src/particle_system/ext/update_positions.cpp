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
	float& angle = render_data.angles_[i];
	angle -= two_pi * std::floor(angle * (1.f / two_pi));
	const int idx = static_cast<int>(angle * angle_to_index) & (ANGLE_TABLE_SIZE - 1);
	const float c = cos_table_[idx], s = sin_table_[idx];

	const float nx = (render_data.positions_x[i] += gamma * c);
	const float ny = (render_data.positions_y[i] += gamma * s);

	render_data.cos_angles_[i] = c;
	render_data.sin_angles_[i] = s;

	const uint32_t slot = spatial_grid.slot_of[i];
	spatial_grid.sorted_x[slot] = nx;
	spatial_grid.sorted_y[slot] = ny;
}

void ParticlePopulation::update_particle_positions()
{
	thread_pool_.run_and_wait();
}