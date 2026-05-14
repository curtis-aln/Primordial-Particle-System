#pragma once

#include <iostream>
#include <SFML/Graphics.hpp>
#include "../utils/spatial_grid/simple_spatial_grid.h"

template<size_t max_beacons, int grid_cells_x, int grid_cells_y>
class Beacons
{
	std::array<size_t, max_beacons> beacons_ = {};
	size_t beacons_size_ = 0;

	// information for finding beacon candidates
	SimpleSpatialGrid* spatial_grid_;
	std::vector<float>* positions_x_;
	std::vector<float>* positions_y_;

	FixedSpan<obj_idx> neighbour_container{ 200 };

	const float cell_size_ = 0.f;
	const float world_width_ = 0.f;
	const float world_height_ = 0.f;

public:
	Beacons(SimpleSpatialGrid* spatial_grid, std::vector<float>* positions_x, std::vector<float>* positions_y,
		const float cell_size, const float world_width, const float world_height)
		: spatial_grid_(spatial_grid), positions_x_(positions_x), positions_y_(positions_y), cell_size_(cell_size), world_width_(world_width), world_height_(world_height)
	{

	}

	void add_beacons(const sf::Vector2f& position, const float radius)
	{
		beacons_size_ = 0;

		const bool out_of_bounds = position.x <= cell_size_ || position.y <= cell_size_ || position.x >= world_width_ - cell_size_ || position.y >= world_height_ - cell_size_;

		if (out_of_bounds)
			return;

		neighbour_container.clear();
		spatial_grid_->find(position.x, position.y, &neighbour_container);

		for (obj_idx index : neighbour_container)
		{
			const sf::Vector2f particle_pos = { (*positions_x_)[index], (*positions_y_)[index] };
			const sf::Vector2f dir = particle_pos - position;
			const float dist = dir.x * dir.x + dir.y * dir.y;

			if (dist < radius * radius)
			{
				beacons_[beacons_size_++] = index;
			}

			if (beacons_size_ >= max_beacons)
			{
				return;
			}
		}
	}

	void render(sf::RenderWindow& window)
	{
		const float rad = PPS_Settings::particle_radius;

		sf::CircleShape beacon_body;
		beacon_body.setFillColor({ 255, 255, 255 });
		beacon_body.setRadius(rad);

		for (int beacon_container_index = 0; beacon_container_index < beacons_size_; ++beacon_container_index)
		{
			int beacon_index = beacons_[beacon_container_index];

			const sf::Vector2f position = { (*positions_x_)[beacon_index] - rad, (*positions_y_)[beacon_index] - rad };
			beacon_body.setPosition(position);
			window.draw(beacon_body);
		}
	}
};