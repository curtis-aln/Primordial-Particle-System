#pragma once
#include <SFML/Graphics.hpp>

#include "utils/random.h"

struct SimulationSettings
{
	inline static constexpr unsigned screen_width = 1900;
	inline static constexpr unsigned screen_height = 1080;
	inline static constexpr auto aspect_ratio = static_cast<float>(screen_width) / static_cast<float>(screen_height);

	inline static constexpr unsigned frame_rate = 60000;
	inline static const sf::Color screen_color = { 0, 0, 0 };
	inline static const std::string window_name = "Primordial Particle Simulation";

	sf::FloatRect sim_bounds = { 0.f, 0.f, screen_width, screen_height };
};

struct SystemSettings
{
	inline static constexpr float scale = 1.f;

	inline static constexpr unsigned particle_count = 5'000;
	//inline static constexpr unsigned particle_count = 3'000;

	inline static constexpr auto hash_cells_y = static_cast< size_t>(14.5f / scale);
	inline static constexpr auto hash_cells_x = static_cast<size_t>((14.5f / scale) * SimulationSettings::aspect_ratio);

	inline static constexpr float radius = 6.f * scale;
	inline static constexpr float visual_radius = radius * 5;

	inline static constexpr float alpha = 180.f;
	inline static constexpr float beta = 17.f;
	inline static constexpr float gamma = radius * 0.67;



	inline static const std::vector <std::pair<unsigned, sf::Color >> colors
	{
		{0, {10, 255, 10, 125}},  // green
		
		{10, {139,69,19, 100}}, // brown
		{15, {0, 0, 255, 100}},  // blue
		{35, {255, 255, 0, 100}}, // yellow
		{40, {255, 0, 0, 100}},// red
		
	};

	//inline static const std::vector <std::pair<unsigned, sf::Color >> colors
	//{
	//	{0, {10, 255, 10, 70}},  // green
	//	{10, {139,69,19, 70}}, // brown
	//	{28, {0, 0, 255, 70}},  // blue
	//	{47, {255, 255, 0, 70}}, // yellow
	//	{59, {200, 0, 0, 70}},// red
	//	{80, {200, 0, 200, 70}},// magenta
	//	{100, {200, 200, 200, 70}},// white
	//
	//};
};
