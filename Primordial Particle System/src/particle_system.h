#pragma once

#include <SFML/Graphics.hpp>
#include "settings.h"
#include "utils/spatial_hash_grid.h"
#include "utils/random.h"
#include "utils/utils.h"


#include <cmath>
#include <random>

#include <immintrin.h> // For AVX instructions
#include <array>


constexpr float pi_div_180 = pi / 180.f;
constexpr size_t cos_sin_table_size = 360;


class NeuralNetwork {
private:
	float weights1[3][2];
	float weights2[3];
	float bias1[3];
	float bias2;
	std::mt19937 gen;
	std::uniform_real_distribution<> dis;

	static inline float clamp(float x) {
		return std::max(-1.0f, std::min(1.0f, x));
	}

public:
	NeuralNetwork() : gen(std::random_device()()), dis(-1, 1) {
		randomize();
	}

	void randomize() {
		const float scale1 = std::sqrt(2.0f / 2);  // He initialization
		const float scale2 = std::sqrt(2.0f / 3);

		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 2; ++j) {
				weights1[i][j] = scale1 * dis(gen);
			}
			weights2[i] = scale2 * dis(gen);
			bias1[i] = scale1 * dis(gen);
		}
		bias2 = scale2 * dis(gen);
	}

	float feedForward(const float input1, const float input2) const {
		float hidden[3];
		for (int i = 0; i < 3; ++i) {
			hidden[i] = clamp(weights1[i][0] * input1 + weights1[i][1] * input2 + bias1[i]);
		}

		float output = bias2;
		for (int i = 0; i < 3; ++i) {
			output += weights2[i] * hidden[i];
		}
		return clamp(output);
	}
};


template<size_t population_size>
class ParticlePopulation : SystemSettings
{
	// used to keep particles within bounds
	sf::FloatRect bounds_{};

	alignas(32) std::array<float, cos_sin_table_size> cos_table;
	alignas(32) std::array<float, cos_sin_table_size> sin_table;

	// Use Structure of Arrays for better cache utilization
	alignas(32) std::array<float, population_size> positions_x;
	alignas(32) std::array<float, population_size> positions_y;
	alignas(32) std::array<float, population_size> angles;
	alignas(32) std::array<uint8_t, population_size> left;
	alignas(32) std::array<uint8_t, population_size> right;

	SpatialHashGrid<hash_cells_x, hash_cells_y> hash_grid;
	sf::Vector2f hash_cell_size;

	// rendering and graphics
	sf::CircleShape circle_drawer{};



public:
	NeuralNetwork network{};


public:
	ParticlePopulation(const sf::FloatRect& bounds) : bounds_(bounds), hash_grid(bounds)
	{
		// initializing particle
		for (size_t i = 0; i < population_size; ++i)
		{
			sf::Vector2f pos = Random::rand_pos_in_rect(bounds);
			positions_x[i] = pos.x;
			positions_y[i] = pos.y;
			angles[i] = Random::rand_range(0.f, 2.f * pi);
			left[i] = 0;
			right[i] = 0;
		}

		// initializing renderer
		circle_drawer.setRadius(radius);

		hash_cell_size = hash_grid.get_cell_size();

		std::cout << hash_cell_size.x << " " << visual_radius << "\n";


	}


	void update()
	{
		// resetting updating the spatial hash grid
		hash_grid.clear();
		for (size_t i = 0; i < population_size; ++i)
		{
			hash_grid.addObject({ positions_x[i], positions_y[i]}, i);
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

			circle_drawer.setPosition({ positions_x[i] - radius, positions_y[i] - radius });
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
		for (size_t i = colors.size() - 1; i >= 0; --i)
		{
			if (left[index] + right[index] >= colors[i].first)
			{
				return colors[i].second;
			}
		}
	}


	void calculate_neighbours(const size_t index)
	{
		// resetting the particle
		left[index] = 0;
		right[index] = 0;

		const sf::Vector2f& position = { positions_x[index], positions_y[index] };
		const c_Vec& near = hash_grid.find(position);

		for (size_t i = 0; i < near.size; i++)
		{
			const int16_t other_index = near.array[i];

			bool should_use_torodial = near.at_border;
			sf::Vector2f other_position = {positions_x[other_index], positions_y[other_index]};

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

	float calculate_b(const int l, const int r)
	{
		return network.feedForward(l/15, r/15) * 180;
		const float result = tanh(l * w1 + r * w2 + (l + r) * w3 + b);
		return (result + 1) * 180.f;
	}

	void update_particle_positioning(const size_t index)
	{
		float& pos_x = positions_x[index];
		float& pos_y = positions_y[index];

		// delta_phi = alpha + beta × N × sign(R - L)
		const unsigned sum = right[index] + left[index]; // TODO do this all at once too

		//const float act_sum = sum <= activation ? sum : 0;
		
		const float delta = alpha + beta * sum * sign(right[index] - left[index]); // calculate_b(left[index], right[index]);
		angles[index] += delta * (pi / 180.f);

		float deltaX = gamma * std::cos(angles[index]);
		float deltaY = gamma * std::sin(angles[index]);

		pos_x += deltaX;
		pos_y += deltaY;

		pos_x = std::fmod(pos_x + bounds_.width, bounds_.width);
		pos_y = std::fmod(pos_y + bounds_.height, bounds_.height);
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
				p_visual_radius.setPosition(sf::Vector2f{positions_x[index] - visual_radius, positions_y[index] - visual_radius});
				window.draw(p_visual_radius);

				window.draw(createLine({ positions_x[index], positions_y[index] }, angles[index], 15));
			}
			
		}

	}

};