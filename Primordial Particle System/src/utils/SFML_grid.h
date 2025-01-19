#pragma once

#include <SFML/Graphics.hpp>
#include<iostream>

// Creates a render_grid_ of nested level N to give a frame of reference when navigating 2d space

// todo use recursion to sort the max nest level
class SFML_Grid
{
	sf::RenderWindow& window_;
	sf::FloatRect world_dimensions_;
	size_t max_nest_level_;

	sf::VertexBuffer vertexBuffer{};

public:
	SFML_Grid(sf::RenderWindow& window, const sf::FloatRect& world_dimensions, const size_t cells_x, const size_t max_nest_level = 3, const sf::Color color = { 75, 75, 75 })
		: window_(window), world_dimensions_(world_dimensions), max_nest_level_(max_nest_level)
	{
		// unpacking data
		const float aspect_ratio = world_dimensions.width / world_dimensions.height;
		const size_t cells_y = cells_x * static_cast<size_t>(aspect_ratio);
		const float cell_size = world_dimensions.width / cells_x;

		std::vector<sf::Vertex> vertices(static_cast<std::vector<sf::Vertex>::size_type>((cells_x + cells_y) * 2));

		vertexBuffer = sf::VertexBuffer(sf::Lines, sf::VertexBuffer::Static);
		vertexBuffer.create(vertices.size());

		size_t counter = 0;
		for (size_t x = 0; x < cells_x; x++)
		{
			const float posX = static_cast<float>(x) * cell_size;
			vertices[counter].position = { posX, 0 };
			vertices[counter + 1].position = { world_dimensions.left + posX, world_dimensions.top + world_dimensions.height };
			counter += 2;
		}

		for (size_t y = 0; y < cells_y; y++)
		{
			const float posY = static_cast<float>(y) * cell_size;
			vertices[counter].position = { 0, posY };
			vertices[counter + 1].position = { world_dimensions.left + world_dimensions.width, world_dimensions.top + posY };
			counter += 2;
		}

		for (size_t x = 0; x < counter; x++)
		{
			vertices[x].color = color;
		}

		vertexBuffer.update(vertices.data(), vertices.size(), 0);
	}


	void draw()
	{
		window_.draw(vertexBuffer);
	}
};
