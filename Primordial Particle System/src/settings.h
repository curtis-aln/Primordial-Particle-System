#pragma once
#include <SFML/Graphics.hpp>

#include "utils/random.h"

struct SimulationSettings
{
	inline static constexpr unsigned screen_width = 1920;
	inline static constexpr unsigned screen_height = 1100;
	inline static constexpr auto aspect_ratio = static_cast<float>(screen_width) / static_cast<float>(screen_height);

	inline static constexpr unsigned frame_rate = 60000;
	inline static const sf::Color screen_color = { 0, 0, 0 };
	inline static const std::string window_name = "Primordial Particle Simulation";

	sf::FloatRect sim_bounds = { 0.f, 0.f, screen_width, screen_height };
};

struct SystemSettings
{
	//inline static constexpr float scale = 1.3f;
	inline static constexpr float scale = .75f;
	//inline static constexpr unsigned particle_count = 10'000;

	inline static constexpr unsigned particle_count = 30'000;

	inline static constexpr auto hash_cells_y = static_cast< size_t>(30.f / scale);
	inline static constexpr auto hash_cells_x = static_cast<size_t>((30.f / scale) * SimulationSettings::aspect_ratio);

	inline static constexpr float param_scale = 3;

	inline static constexpr float radius = 0.8 * param_scale * scale;
	inline static constexpr float visual_radius = 5.f * param_scale * scale;
	inline static constexpr float gamma = 0.67f * param_scale * scale;


	inline static constexpr float alpha = 180.f;
	inline static constexpr float beta = 17.f;



	static const sf::Uint8 transparency = 150;
	inline static const std::vector <std::pair<unsigned, sf::Color >> colors
	{
		{0, {10, 255, 10, transparency}},  // green
		
		{10, {139,69,19, transparency}}, // brown
		{15, {0, 0, 255, transparency}},  // blue
		{35, {255, 255, 0, transparency}}, // yellow
		{40, {255, 0, 255, transparency}},// red
		
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
