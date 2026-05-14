#include "particle_system.h"


thread_local TL_NeighbourPositions neighbour_positions_x;
thread_local TL_NeighbourPositions neighbour_positions_y;


ParticlePopulation::ParticlePopulation(sf::RenderWindow& window) :
	thread_pool(threads), pps_renderer_(&window, &positions_x_, &positions_y_, &angles_, &neighbourhood_count_, particle_radius),
	spatial_grid(grid_cells_x, grid_cells_y, cell_capacity, world_width, world_height)
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
				positions_x_[inc] = col * spacingX + Random::rand11_float() * init_position_scatter;
				positions_y_[inc] = row * spacingY + Random::rand11_float() * init_position_scatter;
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
		positions_x_[index] = position.x;
		positions_y_[index] = position.y;
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
				float& x = positions_x_[i];
				float& y = positions_y_[i];
				if (x < 0.0f || x >= world_width)  x -= world_width * std::floor(x * inv_width_);
				if (y < 0.0f || y >= world_height)  y -= world_height * std::floor(y * inv_height_);
				spatial_grid.add_object(x, y, i); // still needs to be verified thread-safe
			}
			});
	}
	thread_pool.waitForCompletion();
}

void ParticlePopulation::update_particles(const bool paused)
{
	solveCollisions();

	if (!paused)
	{
		update_particle_positions();
	}

}


void ParticlePopulation::render(sf::RenderWindow& window, const bool draw_spatial_grid, const sf::Vector2f pos)
{
	//positions_[0] = pos;
	if (draw_spatial_grid)
	{
		//spatial_grid.render_grid(window); todo
	}

	pps_renderer_.render();
}


void ParticlePopulation::render_debug(sf::RenderWindow& window, const sf::Vector2f mouse_pos, const float debug_radius)
{
	pps_renderer_.render_debug(mouse_pos, debug_radius);
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
	positions_x_.resize(particle_count);
	positions_y_.resize(particle_count);
	angles_.resize(particle_count);
	neighbourhood_count_.resize(particle_count);
}

void ParticlePopulation::randomize_angles()
{
	for (size_t i = 0; i < particle_count; ++i)
	{
		angles_[i] = Random::rand_range(0.f, 2.f * pi);
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
				float& angle = angles_[i];
				const int angle_index = static_cast<int>((angle / two_pi) * ANGLE_TABLE_SIZE) & (ANGLE_TABLE_SIZE - 1);

				angle = fmod(angle, two_pi);
				angle += two_pi * (angle < 0.0f);

				positions_x_[i] += gamma * cos_table_[angle_index];
				positions_y_[i] += gamma * sin_table_[angle_index];
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
		n_positions_x[neighbours_size] = positions_x_[object_index];
		n_positions_y[neighbours_size] = positions_y_[object_index];
		++neighbours_size;
	}
}


inline void ParticlePopulation::update_particle(const obj_idx index, const bool at_border_x, const bool at_border_y,
	std::array<float, cell_capacity * 9>& n_positions_x,
	std::array<float, cell_capacity * 9>& n_positions_y,
	const int neighbours_size)
{
	// first fetch the data we need
	float& x = positions_x_[index];
	float& y = positions_y_[index];
	float& angle = angles_[index];

	// Convert angle to lookup table index
	const int angle_index = static_cast<int>((angle / two_pi) * ANGLE_TABLE_SIZE) & (ANGLE_TABLE_SIZE - 1);
	const float sin_angle = sin_table_[angle_index];
	const float cos_angle = cos_table_[angle_index];

	// calculating the total and right particle count
	int total_neighbours = 0;
	int on_right_hemisphere = 0;

	for (uint32_t i{ 0 }; i < neighbours_size; ++i)
	{
		float direction_x = n_positions_x[i] - x;
		float direction_y = n_positions_y[i] - y;

		if (at_border_x)
		{
			direction_x -= world_width * fast_round(direction_x * inv_width_);
		}

		if (at_border_y)
		{
			direction_y -= world_height * fast_round(direction_y * inv_height_);
		}

		const float dist_sq = direction_x * direction_x + direction_y * direction_y;

		if (dist_sq > 0 && dist_sq < visual_radius * visual_radius)
		{
			on_right_hemisphere += (direction_x * sin_angle - direction_y * cos_angle) < 0;
			++total_neighbours;
		}
	}

	// checking if the direction is on the right of the particle, if so converting this into -1 for false and 1 for trie
	const int left = total_neighbours - on_right_hemisphere;
	const auto sign = static_cast<float>(((on_right_hemisphere - left) >= 0) * 2 - 1);
	neighbourhood_count_[index] = on_right_hemisphere + left;

	angle += (alpha + beta * (on_right_hemisphere + left) * sign) * pi_div_180;
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