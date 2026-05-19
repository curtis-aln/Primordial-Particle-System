#pragma once
#include <SFML/Graphics.hpp>

struct SimulationSettings
{
	static constexpr bool set_Random_seed = false;
	inline static constexpr unsigned screen_width = 1900;
	inline static constexpr unsigned screen_height = 1000;
	inline static constexpr auto aspect_ratio = static_cast<float>(screen_width) / static_cast<float>(screen_height);
	
	inline static constexpr unsigned max_frame_rate = 5200;
	inline static const sf::Color screen_color = { 0, 0, 0 };
	inline static const std::string simulation_title = "Primordial Particle Simulation";

	inline static constexpr bool Vsync = false;
};

struct PPS_Settings
{
	inline static constexpr unsigned threads = 16;
	inline static constexpr unsigned particle_count = 1'000'000;

	// how many frames should pass before the spatial hash grid is updated
	inline static int add_to_grid_freq = 3;

	// scale factors determine how intense / large the difference is
	inline static constexpr float scale_factor = 900;
	inline static constexpr float param_scale_factor = 200.f;

	// world width is the virtual space. screen width is the physical window size
	inline static constexpr auto world_width  = SimulationSettings::screen_width * scale_factor;
	inline static constexpr auto world_height = SimulationSettings::screen_height * scale_factor;

	// calculating how many spatial hash cells should be on each axis
	inline static constexpr auto grid_cells_y = static_cast<size_t>(scale_factor);
	inline static constexpr auto grid_cells_x = static_cast<size_t>((scale_factor) * SimulationSettings::aspect_ratio);
	inline static constexpr int cell_capacity = 20;

	// Scale Sensitive Parameters
	inline static constexpr float visual_radius = 5.f * param_scale_factor;
	inline static constexpr float gamma = 0.67f * param_scale_factor;

	// main simulation rules
	inline static float alpha = 180.f;
	inline static float beta = 13.f;


	// graphical settings
	inline static float particle_radius = 160.f;


	inline static constexpr size_t max_beacon_count = 100;
	inline static constexpr float init_position_scatter = 125.f; // scattering radius of the positions
};