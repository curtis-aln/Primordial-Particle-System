#include "particle_system.h"


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