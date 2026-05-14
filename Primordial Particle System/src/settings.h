#pragma once
#include <SFML/Graphics.hpp>

#include <array>
#include <string>

struct ColorSettings
{
	// Transition thresholds
	inline static float range1 = 10.0f;
	inline static float range2 = 20.0f;
	inline static float range3 = 30.0f;

	// Colors to be mapped to the ranges
	inline static float first_color[3] = { 0.0f, 1.0f, 0.0f };  // Initially Green
	inline static float second_color[3] = { 0.0f, 0.0f, 1.0f }; // Initially Blue
	inline static float third_color[3] = { 1.0f, 1.0f, 0.0f };  // Initially Yellow
	inline static float fourth_color[3] = { 1.0f, 0.0f, 1.0f }; // Initially Magenta
};

struct FontSettings
{
	inline static std::string font_path = "fonts/Calibri.ttf";
	inline static int font_size_debug = 80;
};


struct SimulationSettings
{
	inline static constexpr unsigned screen_width = 1720;
	inline static constexpr unsigned screen_height = 880;
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
	inline static constexpr unsigned particle_count = 100'000;

	inline static constexpr int add_to_grid_freq = 5;

	// scale factors determine how intense / large the difference is
	inline static constexpr float scale_factor = 120;
	inline static constexpr float param_scale_factor = 180.f;

	// world width is the virtual space. screen width is the physical window size
	inline static constexpr auto world_width  = SimulationSettings::screen_width * scale_factor;
	inline static constexpr auto world_height = SimulationSettings::screen_height * scale_factor;

	// calculating how many spatial hash cells should be on each axis
	inline static constexpr auto grid_cells_y = static_cast<size_t>(scale_factor);
	inline static constexpr auto grid_cells_x = static_cast<size_t>((scale_factor) * SimulationSettings::aspect_ratio);

	// Scale Sensitive Parameters
	inline static constexpr float visual_radius = 5.f * param_scale_factor;
	inline static constexpr float gamma = 0.67f * param_scale_factor;


	// graphical settings
	inline static float particle_radius = 100.f;
};


struct Setting
{
	float alpha;
	float beta;
};

struct UpdateRules
{
	// Array of settings with inline comments for descriptive names
	// see https://nagualdesign.github.io/
	inline static const std::array<Setting, 19> settings = { {
		{180.0f, 17.0f},   // 0:  Cell-like behaviour (original)
		{5.0f, 31.0f},     // 1:  Sprites
		{-23.0f, 2.0f},    // 2:  Donuts
		{114.0f, -4.0f},   // 3:  Goblins
		{173.0f, -8.0f},   // 4:  Puff balls
		{22.f, -29.f},     // 5:  Lava Lamp
		{174.0f, 15.0f},   // 6:  Thick cell walls
		{-90.0f, -90.0f},  // 7:  Liquid crystal
		{-20.0f, -25.0f},  // 8:  Gradual evolution
		{146.0f, 8.0f},    // 9:  Thermophiles
		{117.0f, -4.0f},   // 10: Amoebas merging and evolving
		{35.0f, -5.0f},    // 11: Extreme density
		{-70.0f, 5.0f},    // 12: Nascent organelles
		{-177.0f, 3.0f},   // 13: Shockwave
		{-69.0f, -1.0f},   // 14: Rainbow juice
		{42.0f, 9.0f},     // 15: Cell nucleus
		{0.0f, 13.0f},     // 16: Exchanging particles
		{139.0f, -28.0f},  // 17: Silly string
		{79.6f, -0.8f}     // 18: Swirling mass
	} };

	// Default update rule (can be changed dynamically)
	static constexpr int default_rule_index = 13;
	inline static const Setting& update_rules = settings[default_rule_index];

	inline static float alpha = update_rules.alpha;
	inline static float beta = update_rules.beta;
};
