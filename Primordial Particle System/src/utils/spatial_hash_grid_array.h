#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <iostream>
#include <array>

/*
	SpatialHashGrid
- have no more than 65,536 (2^16) objects
- if experiencing error make sure your objects dont go out of bounds
*/


using cell_idx = uint32_t;
using obj_idx = uint32_t;


static constexpr uint8_t cell_capacity = 20;
static constexpr uint8_t max_cell_idx = cell_capacity - 1;

static constexpr uint16_t max_size = cell_capacity * 9;
static constexpr uint16_t max_idx = max_size - 1;



template<size_t cells_x, size_t cells_y>
class SpatialHashGrid
{
public:
	explicit SpatialHashGrid(const sf::FloatRect screenSize = {}) : m_screenSize(screenSize)
	{
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

	}


	// adding an object to the spatial hash grid by a position and storing its id
	void add_object(const sf::Vector2f& pos, const size_t id)
	{
		// mapping the position to the hash grid
		const auto cell_x = static_cast<cell_idx>(pos.x / m_cellSize.x);
		const auto cell_y = static_cast<cell_idx>(pos.y / m_cellSize.y);

		// calculating the 1d index so it can be accessed in memory
		const auto cell_index = static_cast<cell_idx>(cell_x + cell_y * cells_x);

		// adding the atom and incrementing the size
		uint8_t& object_count = objects_count[cell_index];

		objects[cell_index][object_count] = id;
		object_count += object_count < max_cell_idx;
	}


	void find(const sf::Vector2f& position)
	{
		// mapping the position to the hash grid
		const auto cell_x = static_cast<int>(position.x / m_cellSize.x);
		const auto cell_y = static_cast<int>(position.y / m_cellSize.y);

		// calculating the 1d index so it can be accessed in memory
		const auto cell_index = static_cast<cell_idx>(cell_x + cell_y * cells_x);
		
		found_array_size = 0;

		at_border = cell_x == 0 || cell_y == 0 || cell_x == cells_x - 1 || cell_y == cells_y - 1;

		for (int nx = cell_x - 1; nx <= cell_x + 1; ++nx)
		{
			for (int ny = cell_y - 1; ny <= cell_y + 1; ++ny)
			{
				// if the cell is on the border we wrap it
				const auto n_idx_x = !at_border ? nx : (nx + cells_x) % cells_x;
				const auto n_idx_y = !at_border ? ny : (ny + cells_y) % cells_y;

				const auto neighbour_index = n_idx_x + n_idx_y * cells_x;
				const auto& contents = objects[neighbour_index];
				const auto size = objects_count[neighbour_index];

				std::copy(contents.begin(), contents.begin() + size, found_array + found_array_size);
				found_array_size += size;
			
			}
		}
	}


	inline void clear()
	{
		memset(objects_count.data(), 0, total_cells * sizeof(uint8_t));
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

	// graphics
	sf::Vector2f m_cellSize{};
	sf::FloatRect m_screenSize{};


	alignas(32) std::array<uint8_t, total_cells> objects_count;
	alignas(32) std::array<std::array<obj_idx, cell_capacity>, total_cells> objects;



public:
	sf::VertexBuffer vertexBuffer{};

	bool at_border = false;
	obj_idx found_array[max_size] = {};
	uint16_t found_array_size = 0;
};