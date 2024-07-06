#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

#include <cstdint>
#include <iostream>
#include <array>

/*
	SpatialHashGrid
- have no more than 65,536 (2^16) objects
- if experiencing error make sure your objects dont go out of bounds
*/

// when adding an object return its 2d and 1d cell index to use when they want to call find
// todo replace check_valid index with a wrap modulus

using cell_idx = uint32_t;
using obj_idx = uint32_t;


static constexpr uint8_t cell_capacity = 25;
static constexpr uint8_t max_cell_idx = cell_capacity - 1;


template<size_t size>
struct CollisionCells
{
	alignas(32) std::vector<bool> at_border;
	alignas(32) std::vector<uint8_t> object_counts;
	alignas(32) std::vector< std::vector<obj_idx> > objects;
	alignas(32) std::vector<std::vector<cell_idx>> neighbour_indices;
};


struct c_Vec
{
	bool at_border = false;

	static constexpr uint8_t max_size = cell_capacity * 9;
	static constexpr uint8_t max_idx = max_size - 1;

	obj_idx array[max_size] = {};
	uint8_t size = 0;


	inline [[nodiscard]] obj_idx at(const unsigned index) const
	{
		return array[index];
	}
};


template<size_t cells_x, size_t cells_y>
class SpatialHashGrid
{
public:
	explicit SpatialHashGrid(const sf::FloatRect screenSize = {}) : m_screenSize(screenSize)
	{
		collision_cells_.at_border.resize(total_cells);
		collision_cells_.object_counts.resize(total_cells);
		collision_cells_.objects.resize(total_cells, std::vector<obj_idx>(cell_capacity));
		collision_cells_.neighbour_indices.resize(total_cells, std::vector<obj_idx>(9));

		pre_compute_data();
		init_graphics();
		initVertexBuffer();
	}
	~SpatialHashGrid() = default;


	void init_graphics()
	{
		// increasing the size of the boundaries very slightly stops any out-of-range errors 
		constexpr float resize = 0.0001f;
		m_screenSize.left -= resize;
		m_screenSize.top -= resize;
		m_screenSize.width += resize;
		m_screenSize.height += resize;

		m_cellSize = { m_screenSize.width / static_cast<float>(cells_x),
						  m_screenSize.height / static_cast<float>(cells_y) };

		map_conversion_units = { 1.f / m_cellSize.x, 1.f / m_cellSize.y };
	}


	void pre_compute_data()
	{
		// neighbour indexes pre-calculated
		for (size_t x = 0; x < cells_x; ++x)
		{
			for (size_t y = 0; y < cells_y; ++y)
			{
				const auto index = static_cast<cell_idx>(x + y * cells_x);

				// pre-computing if a cell is at the border, toroidal space calculations are expensive
				collision_cells_.at_border[index] = (x == 0 || y == 0 || x == cells_x - 1 || y == cells_y - 1);

				compute_cell_neighbours(index, x, y);
			}
		}
	}

	void compute_cell_neighbours(const cell_idx index, const int x, const int y)
	{
		size_t current_array_index = 0;

		for (int nx = x - 1; nx <= x + 1; ++nx)
		{
			for (int ny = y - 1; ny <= y + 1; ++ny)
			{
				// wrap function
				const auto n_idx_x = (static_cast<size_t>(nx) + cells_x) % cells_x;
				const auto n_idx_y = (static_cast<size_t>(ny) + cells_y) % cells_y;

				const auto neighbour_index = static_cast<cell_idx>(n_idx_x + n_idx_y * cells_x);

				collision_cells_.neighbour_indices[index][current_array_index++] = neighbour_index;
			}
		}
	}


	// adding an object to the spatial hash grid by a position and storing its id
	cell_idx add_object(const sf::Vector2f& pos, const size_t id)
	{
		// mapping the position to the hash grid
		const auto cell_x = static_cast<cell_idx>(pos.x * map_conversion_units.x);
		const auto cell_y = static_cast<cell_idx>(pos.y * map_conversion_units.y);

		// calculating the 1d index so it can be accessed in memory
		const auto cell_index = static_cast<cell_idx>(cell_x + cell_y * cells_x);

		// adding the atom and incrementing the size
		uint8_t& object_count = collision_cells_.object_counts[cell_index];

		collision_cells_.objects[cell_index][object_count] = id;
		object_count += object_count < max_cell_idx;

		return cell_index;
	}


	c_Vec& find(const cell_idx cell_index)
	{
		found.size = 0;

		found.at_border = collision_cells_.at_border[cell_index];

		const auto& neighbour_indices = collision_cells_.neighbour_indices[cell_index];

		for (size_t n = 0; n < 9; ++n)
		{
			const cell_idx neighbour_idx = neighbour_indices[n];
			const uint8_t object_count = collision_cells_.object_counts[neighbour_idx];
			const auto& cell_objects = collision_cells_.objects[neighbour_idx];

			for (uint8_t i = 0; i < object_count; ++i)
			{
				found.array[found.size] = cell_objects[i];
				found.size += found.size < c_Vec::max_idx;
			}
		}

		return found;
	}


	inline void clear()
	{
		// todo optimize
		for (size_t i = 0; i < total_cells; ++i)
		{
			collision_cells_.object_counts[i] = 0;
		}
	}


private:
	void initVertexBuffer()
	{
		std::vector<sf::Vertex> vertices(static_cast<std::vector<sf::Vertex>::size_type>((cells_x + cells_y) * 2));

		vertexBuffer = sf::VertexBuffer(sf::Lines, sf::VertexBuffer::Static);
		vertexBuffer.create(vertices.size());

		size_t counter = 0;
		for (size_t x = 0; x < cells_x; x++)
		{
			const float posX = static_cast<float>(x) * m_cellSize.x;
			vertices[counter].position = { posX, 0 };
			vertices[counter + 1].position = { m_screenSize.left + posX, m_screenSize.top + m_screenSize.height };
			counter += 2;
		}

		for (size_t y = 0; y < cells_y; y++)
		{
			const float posY = static_cast<float>(y) * m_cellSize.y;
			vertices[counter].position = { 0, posY };
			vertices[counter + 1].position = { m_screenSize.left + m_screenSize.width, m_screenSize.top + posY };
			counter += 2;
		}

		for (size_t x = 0; x < counter; x++)
		{
			vertices[x].color = { 75, 75, 75 };
		}

		vertexBuffer.update(vertices.data(), vertices.size(), 0);
	}


private:
	inline static constexpr size_t total_cells = cells_x * cells_y;

	CollisionCells<total_cells> collision_cells_{};

	sf::Vector2f map_conversion_units{};
	c_Vec found{};

	// graphics
	sf::Vector2f m_cellSize{};
	sf::FloatRect m_screenSize{};


public:
	sf::VertexBuffer vertexBuffer{};

	int largest = 0;
};