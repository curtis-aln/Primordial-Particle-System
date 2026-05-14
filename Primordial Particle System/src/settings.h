#pragma once
#include <SFML/Graphics.hpp>

//#include "utils/random.h"
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
	inline static constexpr bool Vsync = false;
};

struct PPS_Settings
{
	// particles   world scale    frame rate
	// 500k        400            10fps
	// 200k        250            60fps
	// 100k        155            140fps
	// 50k         100            300fps
	// 20k         70             840fps
	// 10k         50             1230fps
	// 5k          30             2550fps
	// 1k          15             8500fps


	/*
	particles   world scale    threads   sub_iterations   frame rate
	500k        400            16        1                52fps
	200k        250            16        2                112fps
	100k        160            16        4                320fps
	50k         105            16        8                540fps
	20k         70             16        50               1650fps
	10k         50             16        100              2050fps
	5k          30             8         200              3300fps
	1k          15             4         350              18,500fps
	*/

	// the amount of iterations of the update loop per frame
	inline static constexpr size_t sub_iterations = 1;

	inline static constexpr unsigned threads = 16;
	inline static constexpr unsigned particle_count = 500'000;

	inline static constexpr int add_to_grid_freq = 5;

	// scale factors determine how intense / large the difference is
	inline static constexpr float scale_factor = 400;
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