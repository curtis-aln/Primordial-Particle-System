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
static constexpr uint8_t cell_capacity = 25;

using cellIndex = std::pair < cell_idx, cell_idx>;


template<size_t CellsX, size_t CellsY>
class SpatialHashGrid
{
public:
	explicit SpatialHashGrid(const sf::FloatRect screen_size = {}) : m_screenSize(screen_size)
	{
		init_graphics();
		initVertexBuffer();
		initFont();
	}
	~SpatialHashGrid() = default;


	cellIndex inline hash(const sf::Vector2f pos) const
	{
		const auto cell_x = static_cast<cell_idx>(pos.x / m_cellSize.x);
		const auto cell_y = static_cast<cell_idx>(pos.y / m_cellSize.y);
		return { cell_x, cell_y };
	}


	// adding an object to the spatial hash grid by a position and storing its obj_id
	cellIndex inline add_object(const sf::Vector2f& obj_pos, const size_t obj_id)
	{
		const cellIndex index = hash(obj_pos);

		// adding the atom and incrementing the size
		uint8_t& count = objects_count[index.first][index.second];

		grid[index.first][index.second][count] = static_cast<obj_idx>(obj_id);
		count += count < cell_capacity - 1; // subtracting one prevents going over the size

		return index;
	}


	void find(const sf::Vector2f& position)
	{
		// mapping the position to the grid
		const cellIndex cell_index = hash(position);

		// resetting the found array
		found_array_size = 0;

		// at border is calculated so the object can determine whether to use toroidal wrapping or not
		at_border = cell_index.first == 0 || cell_index.second == 0 || cell_index.first == CellsX - 1 || cell_index.second == CellsY - 1;

		for (int dx = -1; dx <= 1; ++dx)
		{
			for (int dy = -1; dy <= 1; ++dy)
			{
				int index_x = cell_index.first + dx;
				int index_y = cell_index.second + dy;

				if (at_border) // todo
				{
					index_x = (index_x + CellsX) % CellsX;
					index_y = (index_y + CellsY) % CellsY;
				}

				const auto& contents = grid[index_x][index_y];
				const auto size = objects_count[index_x][index_y];

				std::copy(contents.begin(), contents.begin() + size, found_array.begin() + found_array_size);
				found_array_size += size;
			}
		}
	}


	inline void clear()
	{
		memset(objects_count.data(), 0, total_cells * sizeof(uint8_t));
	}


	void render_grid(sf::RenderWindow& window)
	{
		window.draw(vertexBuffer);

		// rendering the locations of each cell with their content counts
		for (int x = 0; x < CellsX; ++x)
		{
			for (int y = 0; y < CellsY; ++y)
			{
				const sf::Vector2f topleft = { x * m_cellSize.x, y * m_cellSize.y };
				text.setString("(" + std::to_string(x) + ", " + std::to_string(y) + ")  obj count: " + std::to_string(objects_count[x][y]));
				text.setPosition(topleft);
				window.draw(text);
			}
		}
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

	void initFont()
	{
		constexpr int char_size = 45;
		const std::string font_location = "fonts/Calibri.ttf";
		if (!font.loadFromFile(font_location))
		{
			std::cerr << "[ERROR]: Failed to load font from: " << font_location << '\n';
			return;
		}
		text = sf::Text("", font, char_size);
	}

	void init_graphics()
	{
		// increasing the size of the boundaries very slightly stops any out-of-range errors 
		constexpr float resize = 1.f;
		m_screenSize.left -= resize;
		m_screenSize.top -= resize;
		m_screenSize.width += resize;
		m_screenSize.height += resize;

		m_cellSize = { m_screenSize.width / static_cast<float>(CellsX),
						  m_screenSize.height / static_cast<float>(CellsY) };

	}


private:
	inline static constexpr size_t total_cells = CellsX * CellsY;

	// graphics
	sf::Vector2f m_cellSize{};
	sf::FloatRect m_screenSize{};

	sf::VertexBuffer vertexBuffer{};
	sf::Font font;
	sf::Text text;

	std::array < std::array<std::array<obj_idx, cell_capacity>, CellsY>,CellsX> grid;
	alignas(32) std::array< std::array<uint8_t, CellsY>, CellsX> objects_count;


public:
	bool at_border = false;
	std::array<obj_idx, cell_capacity * 9> found_array = {};
	uint16_t found_array_size = 0;
};