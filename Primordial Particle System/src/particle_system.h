#pragma once

#include <SFML/Graphics.hpp>
#include "settings.h"
#include "utils/spatial_hash_grid.h"
#include "utils/random.h"
#include "utils/utils.h"


template<size_t population_size>
class ParticlePopulation : SystemSettings
{
	// used to keep particles within bounds
	sf::FloatRect bounds_{};

	// each index represents a particle
	std::array<sf::Vector2f, population_size> positions;
	std::array<float, population_size> angles;
	std::array<uint8_t, population_size> left;
	std::array<uint8_t, population_size> right;

	SpatialHashGrid hash_grid;
	sf::Vector2f hash_cell_size;

	// rendering and graphics
	sf::CircleShape circle_drawer{};


public:
	ParticlePopulation(const sf::FloatRect& bounds)
		: hash_grid(bounds, spatial_hash_dims), bounds_(bounds)
	{
		// initializing particle
		for (size_t i = 0; i < population_size; ++i)
		{
			positions[i] = Random::rand_pos_in_rect(bounds);
			angles[i] = Random::rand_range(0.f, 2.f * pi);
			left[i] = 0;
			right[i] = 0;
		}

		// initializing renderer
		circle_drawer.setRadius(radius);

		hash_cell_size = hash_grid.get_cell_size();
	}


	void update()
	{
		// resetting updating the spatial hash grid
		hash_grid.clear();
		for (size_t i = 0; i < population_size; ++i)
		{
			hash_grid.addObject(positions[i], i);
		}

		// for each particle we calculate its neighbours
		for (size_t i = 0; i < population_size; ++i)
		{
			calculate_neighbours(i);
			update_particle_positioning(i);
		}
	}


	void render(sf::RenderWindow& window, const bool debug = false, const bool draw_hash_grid = false)
	{
		for (size_t i = 0; i < population_size; ++i)
		{
			circle_drawer.setFillColor(get_color(i));

			circle_drawer.setPosition(positions[i] - sf::Vector2f{radius, radius});
			window.draw(circle_drawer, sf::BlendAdd);

			if (debug)
				render_debug(window, i);
		}

		if (draw_hash_grid)
		{
			window.draw(hash_grid.vertexBuffer);
		}
	}


private:
	sf::Color get_color(const size_t index)
	{
		for (size_t i = colors.size() - 2; i >= 0; --i)
		{
			if (left[index] + right[index] >= colors[i].first)
			{
				return colors[i].second;
			}
		}
	}

	bool should_use_toroidal_distance(const sf::Vector2f& position) const
	{
		return (
			position.x < hash_cell_size.x ||
			position.y < hash_cell_size.y ||
			position.x > bounds_.width - hash_cell_size.x ||
			position.y > bounds_.height - hash_cell_size.y);
	}


	void calculate_neighbours(const size_t index)
	{
		// resetting the particle
		left[index] = 0;
		right[index] = 0;

		const sf::Vector2f& position = positions[index];
		const c_Vec& near = hash_grid.find(position);
		const float size = hash_grid.get_cell_size().x;

		for (size_t i = 0; i < near.size; i++)
		{
			bool should_use_torodial = should_use_toroidal_distance(position);
			sf::Vector2f& other_position = positions[near.array[i]];

			sf::Vector2f dir{};
			float dist_sq = 0.f;
			if (should_use_torodial)
			{
				dir = toroidal_direction(position, other_position, bounds_);
				dist_sq = dir.x * dir.x + dir.y * dir.y;
			}
			else
			{
				const sf::Vector2f delta = position - other_position;
				dist_sq = delta.x * delta.x + delta.y * delta.y;
			}

			constexpr float rad_sq = visual_radius * visual_radius;

			if (dist_sq > 0 && dist_sq < rad_sq)
			{
				// Calculate the x and y coordinates of other_pos relative to pos
				sf::Vector2f relative = should_use_torodial ? dir : other_position - position;
				const bool is_on_right = isOnRightHemisphere(relative, angles[index], bounds_, should_use_torodial);

				if (is_on_right) ++right[index];
				else       ++left[index];
			}
		}
	}

	void update_particle_positioning(const size_t index)
	{
		sf::Vector2f& position = positions[index];

		// delta_phi = alpha + beta × N × sign(R - L)
		const unsigned sum = right[index] + left[index]; // TODO do this all at once too
		const float delta = alpha + beta * sum * sign(right[index] - left[index]);
		angles[index] += delta * (pi / 180.f);

		float deltaX = gamma * std::cos(angles[index]);
		float deltaY = gamma * std::sin(angles[index]);

		position += {deltaX, deltaY};

		border(position, bounds_); // TODO AND this
	}


	void render_debug(sf::RenderWindow& window, const size_t index)
	{
		sf::CircleShape p_visual_radius{ visual_radius };
		p_visual_radius.setFillColor({ 0, 0, 0, 0 });
		p_visual_radius.setOutlineThickness(1);
		p_visual_radius.setOutlineColor({ 255 ,255, 255 });

		for (unsigned i = 0; i < population_size; i++)
		{
			if (i % 3 == 0) // one in three
			{
				p_visual_radius.setPosition(positions[index] - sf::Vector2f{visual_radius, visual_radius});
				window.draw(p_visual_radius);

				window.draw(createLine(positions[index], angles[index], 15));
			}
			
		}

	}

};