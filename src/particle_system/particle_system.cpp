#include "particle_system.h"

// Each thread gets their own container of neighbour positions to avoid race conditions
thread_local TL_NeighbourPositions neighbour_positions_x;
thread_local TL_NeighbourPositions neighbour_positions_y;

// Constructor
ParticlePopulation::ParticlePopulation() :
	spatial_grid(grid_cells_x, grid_cells_y, cell_capacity, world_width, world_height),
	thread_pool(initial_thread_count)
{
	precompute_thread_ranges();

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
	thread_pool.setThreadCount(threads);
	precompute_thread_ranges();
}

// Spawns particles in a Grid-Like formation
void ParticlePopulation::init_grid_positioning()
{
	// Calculate the number of columns and rows for a nearly square grid
	int cols = static_cast<int>(std::sqrt(particle_count * (world_width / world_height)));
	int rows = particle_count / cols + (particle_count % cols > 0); // Ensure we cover all particles

	// Calculate the spacing between particles
	const float spacingX = world_width / cols;
	const float spacingY = world_height / rows;

	int inc = 0;
	for (int row = 0; row < rows; ++row)
	{
		for (int col = 0; col < cols; ++col)
		{
			if (inc < particle_count)
			{
				// Setting the positions
				render_data.positions_x[inc] = col * spacingX + Random::rand11_float() * init_position_scatter;
				render_data.positions_y[inc] = row * spacingY + Random::rand11_float() * init_position_scatter;
				inc++;
			}
		}
	}
}

void ParticlePopulation::create_cell_at(const sf::Vector2f position, const int particle_count)
{
	// chooses random particles in the world to concentrate at a certain position.
	for (int _ = 0; _ < particle_count; ++_)
	{
		const int index = Random::rand_range(0u, PPS_Settings::particle_count - 1);
		render_data.positions_x[index] = position.x;
		render_data.positions_y[index] = position.y;
	}
}


void ParticlePopulation::add_particles_to_grid()
{
	spatial_grid.clear();

	for (const auto& [start, end] : grid_insert_ranges_)
	{
		thread_pool.addTask([this, start, end] {
			for (size_t i = start; i < end; ++i)
			{
				float& x = render_data.positions_x[i];
				float& y = render_data.positions_y[i];

				// Wrapping the position to stay within the world
				if (x < 0.0f || x >= world_width)  
					x -= world_width * std::floor(x * inv_width_);
				if (y < 0.0f || y >= world_height)  
					y -= world_height * std::floor(y * inv_height_);
				spatial_grid.add_object(x, y, i); // still needs to be verified thread-safe
			}
			});
	}
	thread_pool.waitForCompletion();
}

void ParticlePopulation::update_particles()
{
  
	if (!toggles.paused)
	{
		if (toggles.use_density_grid)
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

		else
		{
			add_particles_to_grid();
			solveCollisions();
			update_particle_positions();
		}
	}

	iterations_++;
}

void ParticlePopulation::solveCollisions_density()
{
    const uint32_t tc = thread_pool.m_thread_count;
    const int ppt = particle_count / tc;
    for (uint32_t t = 0; t < tc; ++t)
    {
        thread_pool.addTask([this, t, ppt, tc] {
            const int start = t * ppt;
            const int end   = (t == tc - 1) ? particle_count : start + ppt;
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
                if (x < 0.f || x >= world_width)
                    x -= world_width * std::floor(x * inv_width_);
                if (y < 0.f || y >= world_height)
                    y -= world_height * std::floor(y * inv_height_);
            }
        });
    }
    thread_pool.waitForCompletion();
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
	// resizing vectors to the population size
	render_data.positions_x.resize(particle_count);
	render_data.positions_y.resize(particle_count);
	render_data.angles_.resize(particle_count);
	render_data.neighbourhood_count_.resize(particle_count);

	render_data.cos_angles_.resize(particle_count);  
	render_data.sin_angles_.resize(particle_count); 
}

void ParticlePopulation::randomize_angles()
{
	for (size_t i = 0; i < particle_count; ++i)
	{
		render_data.angles_[i] = Random::rand_range(0.f, 2.f * pi);
	}
}


// If the spatial hash grid shape and size is constant, we can pre-calculate the thread information so we dont have to keep
// calculating it every frame
void ParticlePopulation::precompute_thread_ranges()
{
	const uint32_t thread_count = thread_pool.m_thread_count;
	const uint32_t total_cells = grid_cells_x * grid_cells_y;
	const uint32_t slice_size = total_cells / thread_count;

	collision_ranges_.resize(thread_count);
	for (uint32_t i = 0; i < thread_count; ++i)
	{
		collision_ranges_[i].start = i * slice_size;

		// Last thread absorbs the remainder — no separate task needed
		collision_ranges_[i].end = (i == thread_count - 1) ? total_cells : (i + 1) * slice_size;
	}

	// Same for particle grid insertion
	const uint32_t particles_per_thread = particle_count / thread_count;
	grid_insert_ranges_.resize(thread_count);
	for (uint32_t i = 0; i < thread_count; ++i)
	{
		grid_insert_ranges_[i].start = i * particles_per_thread;
		grid_insert_ranges_[i].end = (i == thread_count - 1) ? particle_count : (i + 1) * particles_per_thread;
	}
}

// The snapshot information will get sent off to the renderer to render the frame, transfering data can take a long time however
void ParticlePopulation::fill_snapshot(SimSnapshot& snapshot)
{
	const int n = particle_count;

	// ── Copy raw particle data ─────────────────────────────────────────────
	std::memcpy(snapshot.render.positions_x.data(), render_data.positions_x.data(), n * sizeof(float));
	std::memcpy(snapshot.render.positions_y.data(), render_data.positions_y.data(), n * sizeof(float));
	std::memcpy(snapshot.render.neighbourhood_count_.data(), render_data.neighbourhood_count_.data(), n * sizeof(uint16_t));
	std::memcpy(snapshot.render.angles_.data(), render_data.angles_.data(), n * sizeof(float));

	// ── Precompute cos/sin using the same angle LUT the sim uses ──────────
	// This runs once per sim tick (e.g. 5fps), NOT once per render frame.
	// Table lookup avoids std::cos/sin entirely — just an int cast + array read.
	float* ca = snapshot.render.cos_angles_.data();
	float* sa = snapshot.render.sin_angles_.data();
	const float* angles = render_data.angles_.data();

	for (int i = 0; i < n; ++i)
	{
		const int idx = static_cast<int>((angles[i] / two_pi) * ANGLE_TABLE_SIZE) & (ANGLE_TABLE_SIZE - 1);
		ca[i] = cos_table_[idx];
		sa[i] = sin_table_[idx];
	}

	// ── Statistics ────────────────────────────────────────────────────────
	frame_rate_smoothing_.update_frame_rate();
	snapshot.stats.updating_fps = frame_rate_smoothing_.get_average_frame_rate();
	snapshot.stats.cell_particle_count = PPS_Settings::particle_count;
	snapshot.stats.iterations_ = iterations_;

	const float sim_fps = snapshot.stats.updating_fps;
	snapshot.sim_tick_seconds = (sim_fps > 0.f) ? 1.f / sim_fps : 0.f;
	snapshot.toggles.sampling_mode_ = density_grid.sampling_mode_;
}