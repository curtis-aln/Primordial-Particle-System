#include "../simulation/context/sim_snapshot.h"
#include "particle_system.h"
#include "state.h"
#include <cmath>
#include <cstdint>
#include <settings.h>
#include <SFML/System/Vector2.hpp>
#include <string.h>
#include <utils/random.h>

// Constructor
ParticlePopulation::ParticlePopulation() :
	spatial_grid(grid_cells_x, grid_cells_y, world_width, world_height)
{
	precompute_cell_update_ranges();

	inv_width_ = 1.f / world_width;
	inv_height_ = 1.f / world_height;

	init_particle_vectors();
	init_sin_cos_tables();
	init_grid_positioning();
	randomize_angles();

	// choosing 20 random particles to put at the center
	create_cell_at({ world_width / 2.f, world_height / 2.f }, 35);
}

void ParticlePopulation::randomize_sim()
{
	init_grid_positioning();
	randomize_angles();
}

void ParticlePopulation::set_thread_count(int threads)
{

}

void ParticlePopulation::init_grid_positioning()
{
	RenderData& d = bufs_[cur_];
	int cols = static_cast<int>(std::sqrt(particle_count * (world_width / world_height)));
	int rows = particle_count / cols + (particle_count % cols > 0);
	const float spacingX = world_width / cols;
	const float spacingY = world_height / rows;

	int inc = 0;
	for (int row = 0; row < rows; ++row)
		for (int col = 0; col < cols; ++col)
			if (inc < particle_count)
			{
				d.positions_x[inc] = col * spacingX + Random::rand11_float() * init_position_scatter;
				d.positions_y[inc] = row * spacingY + Random::rand11_float() * init_position_scatter;
				inc++;
			}
	force_rebuild_ = true;
}

void ParticlePopulation::create_cell_at(const sf::Vector2f position, const int count)
{
	RenderData& d = bufs_[cur_];
	for (int k = 0; k < count; ++k)
	{
		const int index = Random::rand_range(0u, PPS_Settings::particle_count - 1);
		d.positions_x[index] = position.x;
		d.positions_y[index] = position.y;
	}
	force_rebuild_ = true;
}

void ParticlePopulation::randomize_angles()
{
	float* a = bufs_[cur_].angles_.data();
	for (size_t i = 0; i < particle_count; ++i)
		a[i] = Random::rand_range(0.f, 2.f * pi);
}


void ParticlePopulation::update_particles()
{
	if (toggles.use_density_grid)
		run_density_grid();
	else
	{
		solve_neighbourhood_detection();
		cur_ ^= 1;
	}

	iterations_++;

	iterations_clock_.tick();
}


void ParticlePopulation::init_sin_cos_tables()
{
	// pre-computing values for the sin and cos tables
	for (int i = 0; i < ANGLE_TABLE_SIZE; ++i)
	{
		float angle = (i / static_cast<float>(ANGLE_TABLE_SIZE)) * two_pi;
		sin_table_[i] = std::sin(angle);
		cos_table_[i] = std::cos(angle);
	}
}

void ParticlePopulation::init_particle_vectors()
{
	for (auto& b : bufs_)
	{
		b.positions_x.resize(particle_count + 8, 0.f);
		b.positions_y.resize(particle_count + 8, 0.f);
		b.angles_.resize(particle_count);
		b.neighbourhood_count_.resize(particle_count);
	}
}


// The snapshot information will get sent off to the renderer to render the frame, transfering data can take a long time however
void ParticlePopulation::fill_snapshot(SimSnapshot& snapshot)
{
	const int n = particle_count;
	const RenderData& src = bufs_[cur_];

	std::memcpy(snapshot.render.positions_x.data(), src.positions_x.data(), n * sizeof(float));
	std::memcpy(snapshot.render.positions_y.data(), src.positions_y.data(), n * sizeof(float));
	std::memcpy(snapshot.render.neighbourhood_count_.data(), src.neighbourhood_count_.data(), n * sizeof(uint16_t));
	std::memcpy(snapshot.render.angles_.data(), src.angles_.data(), n * sizeof(float));

	const float* a = src.angles_.data();
	float* ca = snapshot.render.cos_angles_.data(); float* sa = snapshot.render.sin_angles_.data();
	for (int i = 0; i < n; ++i)
	{
		const int idx = static_cast<int>(a[i] * angle_to_index) & (ANGLE_TABLE_SIZE - 1);
		ca[i] = cos_table_[idx]; sa[i] = sin_table_[idx];
	}


	snapshot.stats.cell_particle_count = PPS_Settings::particle_count;
	snapshot.stats.iterations_ = iterations_;
	snapshot.stats.iterations_per_second = iterations_clock_.get_average_frame_rate();

	const float sim_fps = snapshot.stats.updating_fps;
	snapshot.sim_tick_seconds = (sim_fps > 0.f) ? 1.f / sim_fps : 0.f;
	snapshot.toggles.sampling_mode_ = density_grid.sampling_mode_;
}