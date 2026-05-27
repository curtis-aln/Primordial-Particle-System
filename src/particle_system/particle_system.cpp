#include "particle_system.h"

// Each thread gets their own container of neighbour positions to avoid race conditions
thread_local TL_NeighbourPositions neighbour_positions_x;
thread_local TL_NeighbourPositions neighbour_positions_y;


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

	add_particles_to_grid();
	density_grid.build(
		render_data.positions_x.data(),
		render_data.positions_y.data(),
		particle_count);
	
	    
	if (!toggles.paused)
	{
		if (toggles.use_density_grid)
			solveCollisions_density();
		else
		{
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


void ParticlePopulation::update_particle_positions()
{
	// updating the positions of each particles in the direction of their angle by step size `gamma`
	const uint32_t thread_count = thread_pool.m_thread_count;
	const int particles_per_thread = particle_count / thread_count;
	const int last_thread_particles = particle_count - (thread_count - 1) * particles_per_thread;

	for (uint32_t t = 0; t < thread_count; ++t)
	{
		thread_pool.addTask([this, t, particles_per_thread, last_thread_particles, thread_count] {
			const int start = t * particles_per_thread;
			const int end = (t == thread_count - 1) ? start + last_thread_particles : start + particles_per_thread;

			for (int i = start; i < end; ++i)
			{
				// Update position
				float& angle = render_data.angles_[i];
				const int angle_index = static_cast<int>((angle / two_pi) * ANGLE_TABLE_SIZE) & (ANGLE_TABLE_SIZE - 1);

				angle = fmod(angle, two_pi);
				angle += two_pi * (angle < 0.0f);

				render_data.positions_x[i] += gamma * cos_table_[angle_index];
				render_data.positions_y[i] += gamma * sin_table_[angle_index];
			}
			});
	}

	thread_pool.waitForCompletion();
}


void ParticlePopulation::solveCollisions()
{
	for (const auto& [start, end] : collision_ranges_)
	{
		thread_pool.addTask([this, start, end] {
			for (uint32_t idx{ start }; idx < end; ++idx)
				process_cell(idx, neighbour_positions_x, neighbour_positions_y);
		});
	}
	thread_pool.waitForCompletion();
}


void ParticlePopulation::process_cell(
	const cell_idx cell_index,
	std::array<float, cell_capacity * 9>& n_positions_x,
	std::array<float, cell_capacity * 9>& n_positions_y)
{
	// for a given cell this function will access its particle contents. and for each one of them it will update them based off the information from the
	// neighbouring 9 cells.
	int neighbours_size = 0;

	const int cell_index_x = cell_index % grid_cells_x;
	const int cell_index_y = cell_index / grid_cells_x;
	const bool at_border_x = cell_index_x == 0 || cell_index_x == grid_cells_x - 1;
	const bool at_border_y = cell_index_y == 0 || cell_index_y == grid_cells_y - 1;

	// each possible neighbour in the 3x3 area
	add_neighbour_cells_particles(n_positions_x, n_positions_y, neighbours_size, cell_index_x - 1, cell_index_y - 1, true, true);
	add_neighbour_cells_particles(n_positions_x, n_positions_y, neighbours_size, cell_index_x, cell_index_y - 1, false, true);
	add_neighbour_cells_particles(n_positions_x, n_positions_y, neighbours_size, cell_index_x + 1, cell_index_y - 1, true, true);
	add_neighbour_cells_particles(n_positions_x, n_positions_y, neighbours_size, cell_index_x - 1, cell_index_y, true, false);
	add_neighbour_cells_particles(n_positions_x, n_positions_y, neighbours_size, cell_index_x, cell_index_y, false, false);
	add_neighbour_cells_particles(n_positions_x, n_positions_y, neighbours_size, cell_index_x + 1, cell_index_y, true, false);
	add_neighbour_cells_particles(n_positions_x, n_positions_y, neighbours_size, cell_index_x - 1, cell_index_y + 1, true, true);
	add_neighbour_cells_particles(n_positions_x, n_positions_y, neighbours_size, cell_index_x, cell_index_y + 1, false, true);
	add_neighbour_cells_particles(n_positions_x, n_positions_y, neighbours_size, cell_index_x + 1, cell_index_y + 1, true, true);

	// updating the particles
	const auto* contents = &spatial_grid.grid[cell_index * spatial_grid.cell_max_capacity];
	const uint8_t cell_size = spatial_grid.cell_capacities[cell_index];

	for (uint8_t idx = 0; idx < cell_size; ++idx)
	{
		update_particle(contents[idx], at_border_x, at_border_y, n_positions_x, n_positions_y, neighbours_size);
	}

}


inline void ParticlePopulation::add_neighbour_cells_particles(
	std::array<float, cell_capacity * 9>& n_positions_x,
	std::array<float, cell_capacity * 9>& n_positions_y,
	int& neighbours_size,
	int32_t neighbour_index_x, int32_t neighbour_index_y,
	bool check_x,
	bool check_y)
{
	// Fast modulo for positive and negative numbers
	if (check_x)
	{
		neighbour_index_x = neighbour_index_x >= 0 ?
			(neighbour_index_x < grid_cells_x ? neighbour_index_x : neighbour_index_x - grid_cells_x) :
			(neighbour_index_x + grid_cells_x);
	}

	if (check_y)
	{
		neighbour_index_y = neighbour_index_y >= 0 ?
			(neighbour_index_y < grid_cells_y ? neighbour_index_y : neighbour_index_y - grid_cells_y) :
			(neighbour_index_y + grid_cells_y);
	}

	// fetching data for copying
	const uint32_t neighbour_index = neighbour_index_y * grid_cells_x + neighbour_index_x;

	const auto* contents = &spatial_grid.grid[neighbour_index * spatial_grid.cell_max_capacity];

	const auto size = spatial_grid.cell_capacities[neighbour_index];

	// adding the neighbour data to the array
	for (uint8_t idx = 0; idx < size; ++idx)
	{
		const obj_idx object_index = contents[idx];
		n_positions_x[neighbours_size] = render_data.positions_x[object_index];
		n_positions_y[neighbours_size] = render_data.positions_y[object_index];
		++neighbours_size;
	}
}


inline void ParticlePopulation::update_particle(
	const obj_idx index, const bool at_border_x, const bool at_border_y,
	const std::array<float, cell_capacity * 9>& n_positions_x,
	const std::array<float, cell_capacity * 9>& n_positions_y,
	const int neighbours_size)
{
	const float x = render_data.positions_x[index];
	const float y = render_data.positions_y[index];
	float& angle = render_data.angles_[index];

	const int   angle_index = static_cast<int>((angle / two_pi) * ANGLE_TABLE_SIZE)
		& (ANGLE_TABLE_SIZE - 1);
	const float sin_a = sin_table_[angle_index];
	const float cos_a = cos_table_[angle_index];

	static constexpr float r_sq = visual_radius * visual_radius;

	int total = 0;
	int on_right = 0;

	// ── Fast path (vast majority of cells) ────────────────────────────────
	if (!at_border_x && !at_border_y) [[likely]]
	{
		for (int i = 0; i < neighbours_size; ++i)
		{
			const float dx = n_positions_x[i] - x;
			const float dy = n_positions_y[i] - y;
			const float d_sq = dx * dx + dy * dy;

			if (d_sq > 0.f & d_sq < r_sq)   // bitwise & avoids branch
			{
				on_right += (dx * sin_a - dy * cos_a) < 0.f;
				++total;
			}
		}
	}
	// ── Slow path (border cells only) ─────────────────────────────────────
	else
	{
		for (int i = 0; i < neighbours_size; ++i)
		{
			float dx = n_positions_x[i] - x;
			float dy = n_positions_y[i] - y;
			if (at_border_x) dx -= world_width * fast_round(dx * inv_width_);
			if (at_border_y) dy -= world_height * fast_round(dy * inv_height_);
			const float d_sq = dx * dx + dy * dy;

			if (d_sq > 0.f & d_sq < r_sq)
			{
				on_right += (dx * sin_a - dy * cos_a) < 0.f;
				++total;
			}
		}
	}

	// on_right + left == on_right + (total - on_right) == total
	render_data.neighbourhood_count_[index] = static_cast<uint16_t>(total);

	const int   left = total - on_right;
	const float sign = static_cast<float>(((on_right - left) >= 0) * 2 - 1);
	angle += (alpha + beta * static_cast<float>(total) * sign) * pi_div_180;
}

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


// Only the fill_snapshot function is shown — everything else in particle_system.cpp is unchanged.

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
}