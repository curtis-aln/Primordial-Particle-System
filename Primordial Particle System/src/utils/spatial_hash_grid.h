#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

#include <cstdint>
#include <iostream>
#include <array>

/*
	SpatialHashGrid

NOTE:
- have no more than 65,536 (2^16) objects

	Improvements:
	- Replace std::vector<CollisionCell> m_cells{} with a fixed-size array for betetr cashe locality.
	- pre-compute cell neighbours
	- pre-compute if cells are at the border
	- pre-compute 2d indexes for cells
	- object of arrays
	- object counts might be -1 what it should be
	- c_Vec size may be wrong

*/


template<typename T>
void grow_rect(sf::Rect<T>& rect, const T buffer)
{
	rect.left -= buffer;
	rect.top -= buffer;
	rect.width += buffer;
	rect.height += buffer;
}


// cell_capacity is the absolute MAXIMUM amount of objects that will be in this cell
static constexpr uint16_t cell_capacity = 16;
static constexpr uint16_t max_cell_idx = cell_capacity - 1;


template<size_t size>
struct CollisionCells
{
	bool at_border[size] = {};
	uint8_t object_counts[size] = {};
	uint16_t objects[size][cell_capacity] = {};
	uint16_t neighbour_indices[size][9] = {};
};


struct c_Vec
{
	bool at_border = false;

	static constexpr uint8_t max = cell_capacity * 9;
	int16_t array[max] = {};
	uint8_t size = 0;

	void add(const int16_t value)
	{
		if (size >= max)
			return;

		array[size++] = value;
	}

	inline [[nodiscard]] int16_t at(const unsigned index) const
	{
		return array[index];
	}
};


template<size_t cells_x, size_t cells_y>
class SpatialHashGrid
{
public:
	// constructor and destructor
	explicit SpatialHashGrid(const sf::FloatRect screenSize = {})
	{
		// calculating the neighbouring indices at creation so it won't have to be re-calculated every frame
		for (int32_t x = 0; x < cells_x; ++x)
		{
			for (int32_t y = 0; y < cells_y; ++y)
			{
				const int16_t index = idx2d_to1d({ x, y });
				size_t current_array_index = 0;

				collision_cells_.at_border[index] = (x == 0 || y == 0 || x == cells_x - 1 || y == cells_y - 1);

				for (int32_t nx = -1; nx <= 1; ++nx)
				{
					for (int32_t ny = -1; ny <= 1; ++ny)
					{
						int32_t n_idx_x = x + nx;
						int32_t n_idx_y = y + ny;

						// wrap function
						n_idx_x = (n_idx_x + cells_x) % cells_x;
						n_idx_y = (n_idx_y + cells_y) % cells_y;


						const int16_t n_index = idx2d_to1d({ n_idx_x, n_idx_y});


						collision_cells_.neighbour_indices[index][current_array_index++] = n_index;
					}
				}
			}
		}


		m_screenSize = screenSize;

		// this accounts for floating point innaccurate scenarios where we are adding an Object which is right on the border of
		// the screen_bounds of the spatial hash grid.
		grow_rect(m_screenSize, 0.001f);

		m_cellSize = { m_screenSize.width / static_cast<float>(cells_x),
						  m_screenSize.height / static_cast<float>(cells_y) };

		map_conversion_units = { 1.f / m_cellSize.x, 1.f / m_cellSize.y };

		initVertexBuffer();
	}
	~SpatialHashGrid() = default;


	// adding an object to the spatial hash grid by a position and storing its id
	void addObject(const sf::Vector2f& pos, const int32_t id)
	{
		// mapping its position to the hash grid
		const sf::Vector2<int32_t> cIdx = pos_to2d_idx(pos);

		if (!check_valid_index(cIdx))
			throw std::out_of_range("find() position argument out of range");

		const int32_t idx = idx2d_to1d(cIdx);

		// adding the atom
		uint8_t& object_count = collision_cells_.object_counts[idx];

		collision_cells_.objects[idx][object_count] = id;
		object_count += object_count < max_cell_idx;

	}


	c_Vec& find(const sf::Vector2f& position)
	{
		found.size = 0;

		const sf::Vector2<int32_t> cIdx = pos_to2d_idx(position);
		const uint16_t cell_index = idx2d_to1d(cIdx);

		if (!check_valid_index(cIdx))
			throw std::out_of_range("find() position argument out of range");

		for (uint16_t neighbour_idx : collision_cells_.neighbour_indices[cell_index])
		{
			const uint16_t object_count = collision_cells_.object_counts[neighbour_idx];

			for (uint8_t i = 0; i < object_count; ++i)
			{
				found.add(collision_cells_.objects[neighbour_idx][i]);
			}
		}

		found.at_border = collision_cells_.at_border[cell_index];
		return found;
	}


	inline void clear()
	{
		for (size_t i = 0; i < total_cells; ++i)
		{
			collision_cells_.object_counts[i] = 0;
		}
	}


private:
	static inline [[nodiscard]] int32_t idx2d_to1d(const sf::Vector2<int32_t> idx)
	{
		return idx.x + idx.y * cells_x;
	}

	inline [[nodiscard]] sf::Vector2<int32_t> pos_to2d_idx(const sf::Vector2f position) const
	{
		return {
			static_cast<int32_t>(position.x * map_conversion_units.x),
			static_cast<int32_t>(position.y * map_conversion_units.y) };
	}


	static inline [[nodiscard]] bool check_valid_index(const sf::Vector2i index)
	{
		return !(index.x < 0 || index.y < 0 || index.x >= cells_x || index.y >= cells_y);
	}


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

		vertexBuffer.update(vertices.data(), vertices.size(), 0);
	}

public:
	inline sf::Vector2f get_cell_size() { return m_cellSize; }



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
};