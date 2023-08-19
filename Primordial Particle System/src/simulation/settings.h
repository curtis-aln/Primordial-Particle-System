#pragma once
#include <SFML/Graphics.hpp>

struct SimulationSettings
{
	inline static constexpr unsigned particle_count = 2500;

	inline static constexpr unsigned screen_width = 1800;
	inline static constexpr unsigned screen_height = 1100;

	inline static constexpr unsigned frame_rate = 100;
	inline static const sf::Color screen_color = { 0, 0, 0 };
	inline static const std::string window_name = "Primodoral Particle Simulation";
};

struct SystemSettings
{
	inline static constexpr float radius = 4.5f;
	inline static constexpr float visual_radius = radius * 10.f;

	inline static constexpr float alpha  = 180.f;
	inline static constexpr float beta   = 17.f;
	inline static constexpr float gamma  = radius/2;

	inline static constexpr std::vector<sf::Color> colors
	{

	};
};
