#pragma once
#include <SFML/Graphics.hpp>

struct SimulationSettings
{
	inline static constexpr unsigned screen_width = 1800;
	inline static constexpr unsigned screen_height = 1000;

	inline static constexpr unsigned frame_rate = 16;
	inline static const sf::Color screen_color = { 0, 0, 0 };
	inline static const std::string window_name = "Primodoral Particle Simulation";

	

	sf::Rect<float> screen_bounds = { 0.f, 0.f, static_cast<float>(screen_width), static_cast<float>(screen_height) };
};

struct SystemSettings
{
	inline static constexpr unsigned particle_count = 10'000;
	inline static const sf::Vector2u spatial_hash_dims = { 40, 30 };

	inline static constexpr float scale = 4.f;
	
	inline static constexpr float visual_radius = 5.f * scale;

	inline static constexpr float alpha = 180.f;
	inline static constexpr float beta  = 17.f;
	inline static constexpr float gamma = 0.67f * scale;

	// rendering
	inline static constexpr float radius = 0.67f * .5f * scale;
	inline static constexpr float magenta_radius = radius * 5.f;
	inline static const std::vector <std::pair<unsigned, sf::Color >> colors
	{
		{0, {0, 255, 0, 125}},  // green
		{13, {139,69,19, 125}}, // brown
		{15, {0, 0, 255, 125}},  // blue
		{35, {255, 255, 0, 125}}, // yellow
		{15, {255, 0, 255, 125}},// magenta
		
	};
};
