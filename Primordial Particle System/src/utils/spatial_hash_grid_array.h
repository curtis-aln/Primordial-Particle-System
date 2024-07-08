#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <iostream>
#include <array>

/*
	SpatialHashGrid
- have no more than 65,536 (2^16) objects
- if experiencing error make sure your objects don't go out of bounds
*/

// make cell grid 2d
// tell objects what index they are in
// instead of removing and re-adding every frame. only remove and re-add if a object changes its cell

// resize these to either
// - uint32_t (4.2 billion max cells/objects, 65,000 x 65,000 grid)
// - uint16_t (65,536 max cells/objects, 256 x 256 grid)
using cell_idx = uint16_t;
using obj_idx = uint16_t;

// maximum number of objects a cell can hold
static constexpr uint8_t cell_capacity = 30;
static constexpr uint16_t max_size = cell_capacity * 9;

using cellIndex = std::pair < cell_idx, cell_idx>;


template<size_t CellsX, size_t CellsY>
class SpatialHashGridOptimized
{
public:
	explicit SpatialHashGridOptimized(const sf::FloatRect screen_size = {}) : m_screenSize(screen_size)
	{
		init_graphics();
		initVertexBuffer();
	}
	~SpatialHashGridOptimized() = default;


	void init_graphics()
	{
		// increasing the size of the boundaries very slightly stops any out-of-range errors 
		constexpr float resize = 0.0001f;
		m_screenSize.left -= resize;
		m_screenSize.top -= resize;
		m_screenSize.width += resize;
		m_screenSize.height += resize;

		m_cellSize = { m_screenSize.width / static_cast<float>(CellsX),
						  m_screenSize.height / static_cast<float>(CellsY) };

	}


	cellIndex inline hash(const sf::Vector2f pos) const
	{
		const auto cell_x = static_cast<cell_idx>(pos.x / m_cellSize.x);
		const auto cell_y = static_cast<cell_idx>(pos.y / m_cellSize.y);
		return { cell_x, cell_y };
	}


	// adding an object to the spatial hash grid by a position and storing its obj_id
	void add_object(const sf::Vector2f& obj_pos, const size_t obj_id)
	{
		const cellIndex index = hash(obj_pos);

		// adding the atom and incrementing the size
		uint8_t& count = objects_count[index.first][index.second];

		grid[index.first][index.second][count] = obj_id;
		count += count < cell_capacity;
	}


	void find(const sf::Vector2f& position)
	{
		// mapping the position to the grid
		const cellIndex cell_index = hash(position);

		// resetting the found array
		found_array_size = 0;

		// at border is calculated so the object can determine whether to use toroidal wrapping or not
		at_border = cell_index.first == 0 || cell_index.second == 0 || cell_index.first == CellsX - 1 || cell_index.second == CellsY - 1;

		for (int dx = -1; dx != 2; ++dx)
		{
			for (int dy = -1; dy != 2; ++dy)
			{
				const int index_x = (cell_index.first + dx + CellsX) % CellsX;
				const int index_y = (cell_index.second + dy + CellsY) % CellsY;

				const auto& contents = grid[index_x][index_y];
				const auto size = objects_count[index_x][index_y];

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
		std::vector<sf::Vertex> vertices(static_cast<std::vector<sf::Vertex>::size_type>((CellsX + CellsY) * 2));

		vertexBuffer = sf::VertexBuffer(sf::Lines, sf::VertexBuffer::Static);
		vertexBuffer.create(vertices.size());

		size_t counter = 0;
		for (size_t x = 0; x < CellsX; x++)
		{
			const float posX = static_cast<float>(x) * m_cellSize.x;
			vertices[counter].position = { posX, 0 };
			vertices[counter + 1].position = { m_screenSize.left + posX, m_screenSize.top + m_screenSize.height };
			counter += 2;
		}

		for (size_t y = 0; y < CellsY; y++)
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
	inline static constexpr size_t total_cells = CellsX * CellsY;

	// graphics
	sf::Vector2f m_cellSize{};
	sf::FloatRect m_screenSize{};

	using cell_container = std::array<cell_idx, cell_capacity>;

	std::array < std::array<cell_container, CellsY>,CellsX> grid;
	alignas(32) std::array< std::array<uint8_t, CellsY>, CellsX> objects_count;


public:
	sf::VertexBuffer vertexBuffer{};

	bool at_border = false;
	obj_idx found_array[max_size] = {};
	uint16_t found_array_size = 0;
};