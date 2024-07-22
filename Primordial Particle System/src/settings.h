#pragma once
#include <SFML/Graphics.hpp>

#include "utils/random.h"
#include "utils/spatial_hash_grid.h"

struct SimulationSettings
{
	inline static constexpr unsigned screen_width = 1900;
	inline static constexpr unsigned screen_height = 1000;
	inline static constexpr auto aspect_ratio = static_cast<float>(screen_width) / static_cast<float>(screen_height);
	
	inline static constexpr unsigned max_frame_rate = 5200;
	inline static const sf::Color screen_color = { 0, 0, 0 };
	inline static const std::string simulation_title = "Primordial Particle Simulation";

	inline static constexpr bool record = false; // for recording timelapses
};

struct PPS_Settings
{
	inline static constexpr unsigned particle_count = 200'000;

	// scale factors determine how intense / large the difference is
	inline static constexpr float scale_factor = 250;
	inline static constexpr float param_scale_factor = 200.f;

	// world width is the virtual space. screen width is the physical window size
	inline static constexpr auto world_width  = static_cast<float>(SimulationSettings::screen_width) * scale_factor;
	inline static constexpr auto world_height = static_cast<float>(SimulationSettings::screen_height) * scale_factor;

	// calculating how many spatial hash cells should be on each axis
	inline static constexpr auto grid_cells_y = static_cast<size_t>(scale_factor);
	inline static constexpr auto grid_cells_x = static_cast<size_t>((scale_factor) * SimulationSettings::aspect_ratio);

	// Scale Sensitive Parameters
	inline static constexpr float visual_radius = 5.f * param_scale_factor;
	inline static constexpr float gamma = 0.67f * param_scale_factor;

	// main simulation rules
	inline static constexpr float alpha = 180.f;
	inline static constexpr float beta = 17.f;
};



struct PPS_Graphics
{
	inline static constexpr float particle_radius = 110.f;


	// the colors of each particle density
	static constexpr sf::Uint8 transparency = 210;
	inline static const std::vector <std::pair<unsigned, sf::Color >> colors
	{
		{0, {10, 255, 50, transparency}},  // green

		{10, {139,69,19, transparency}}, // brown
		{15, {0, 0, 255, transparency}},  // blue
		{35, {255, 255, 0, transparency}}, // yellow
		{40, {255, 0, 255, transparency}},// magenta
		{cell_capacity * 5 , {255, 0, 0, transparency}},// red

	};
};