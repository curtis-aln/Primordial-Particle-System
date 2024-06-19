#pragma once
#include <SFML/Graphics.hpp>

struct SimulationSettings
{
	inline static constexpr unsigned screen_width = 1800;
	inline static constexpr unsigned screen_height = 1000;

	inline static constexpr unsigned frame_rate = 3000;
	inline static const sf::Color screen_color = { 0, 0, 0 };
	inline static const std::string window_name = "Primordial Particle Simulation";

	sf::FloatRect sim_bounds = { 0.f, 0.f, screen_width, screen_height };
};

struct SystemSettings
{
	inline static constexpr unsigned particle_count = 5000;
	inline static const sf::Vector2u spatial_hash_dims = { 30, 20 };

	inline static constexpr float scale = 10.f;
	inline static constexpr float visual_radius = 3.f * scale;
	inline static constexpr float gamma = 0.67f * scale;
	inline static constexpr float radius = 0.67f * .5f * scale;
	inline static constexpr float magenta_radius = radius * 5.f;
	inline static const std::vector <std::pair<unsigned, sf::Color >> colors
	{
		{0, {0, 255, 0, 125}},  // green
		//{0, {0, 0, 0, 0} },
		{13, {139,69,19, 125}}, // brown
		{15, {0, 0, 255, 125}},  // blue
		{35, {255, 255, 0, 125}}, // yellow
		{15, {255, 0, 255, 125}},// magenta
		
	};
};
