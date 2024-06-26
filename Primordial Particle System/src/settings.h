#pragma once
#include <SFML/Graphics.hpp>

struct SimulationSettings
{
	inline static constexpr unsigned screen_width = 1800;
	inline static constexpr unsigned screen_height = 1000;
	inline static constexpr auto aspect_ratio = static_cast<float>(screen_width) / static_cast<float>(screen_height);

	inline static constexpr unsigned frame_rate = 60000;
	inline static const sf::Color screen_color = { 0, 0, 0 };
	inline static const std::string window_name = "Primordial Particle Simulation";

	sf::FloatRect sim_bounds = { 0.f, 0.f, screen_width, screen_height };
};

struct SystemSettings
{
	inline static constexpr float scale = 0.34f;

	inline static constexpr unsigned particle_count = 8'000;

	inline static unsigned hash_dims_height = 10.f / scale;
	inline static const sf::Vector2u spatial_hash_dims = {
		static_cast<unsigned>(static_cast<float>(hash_dims_height) * SimulationSettings::aspect_ratio),
		hash_dims_height};

	inline static constexpr float radius = 12.f * scale;
	inline static constexpr float visual_radius = radius * 5;

	inline static constexpr float gamma = radius * 0.67;


	inline static const std::vector <std::pair<unsigned, sf::Color >> colors
	{
		{0, {0, 255, 0, 125}},  // green
		
		{10, {139,69,19, 100}}, // brown
		{15, {0, 0, 255, 100}},  // blue
		{35, {255, 255, 0, 100}}, // yellow
		{15, {255, 0, 255, 100}},// magenta
		
	};
};
