#pragma once
#include <SFML/Graphics.hpp>

#include "utils/spatial_grid.h"

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
	/*
	particles   world scale    threads   sub_iterations   frame rate
	4m          650            16        1                11fps
	1m          550            16        1                60fps
	500k        400            16        1                ?
	200k        250            16        2                ?
	100k        160            16        4                ?
	50k         105            16        8                ?
	20k         70             16        50               ?
	10k         50             16        100              ?
	5k          30             8         200              ?
	1k          15             4         350              ?
	*/

	// the amount of iterations of the update loop per frame
	inline static constexpr size_t sub_iterations = 1;

	inline static constexpr unsigned threads = 16;
	inline static constexpr unsigned particle_count = 4'000'000;

	inline static constexpr int add_to_grid_freq = 5;

	// scale factors determine how intense / large the difference is
	inline static constexpr float scale_factor = 950;
	inline static constexpr float param_scale_factor = 200.f;

	// world width is the virtual space. screen width is the physical window size
	inline static constexpr auto world_width  = SimulationSettings::screen_width * scale_factor;
	inline static constexpr auto world_height = SimulationSettings::screen_height * scale_factor;

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
	inline static constexpr float particle_radius = 150.f;
};