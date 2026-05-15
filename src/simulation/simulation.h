#pragma once
#include "settings.h"
#include "particle_system/particle_system.h"
#include "utils/smooth_frame_rates.h"
#include "utils/font.h"
#include "utils/Camera.hpp"
#include "utils/SFML_grid.h"

#include "context/triple_buffer.h"
#include "context/sim_snapshot.h"
#include "context/sim_command.h" 

#include <string>

#include "../utils/stop_watch.h"

class Simulation : PPS_Settings, SimulationSettings
{
	// SFML
	sf::RenderWindow window_{};

	// Smooths Frame rates by averaging them
	FrameRateSmoothing<10> clock_{};

	// Allows for translation & Zooming
	Camera camera{ &window_, 1.f / scale_factor };

	// Two separate font sizes. allows rendering of text on-screen
	Font title_font = { &window_, 60, "fonts/Calibri.ttf" };
	Font text_font = { &window_, 35, "fonts/Calibri.ttf" };

	// Runtime variables and statistics
	StopWatch m_delta_time_{};
	size_t iterations_ = 0;
	float    m_total_time_elapsed_ = 0.f;

	bool running_ = true;
	bool render_hash_grid_ = false;
	bool debug_ = false;
	bool rendering_ = true;

	float fps_ = 0.f;

	// radius around the mouse in which debug settings are shown
	float debug_radius = 8000.f;
	const float change_in_debug_radius = 500.f;
	SFML_Grid grid{ window_, sf::FloatRect({0, 0}, {world_width, world_height}), 10 };

	// The particle system
	ParticlePopulation particle_system_{};
	PPS_Renderer pps_renderer_{&window_};

	// the render-update seperated multithreadding
	TripleBuffer<SimSnapshot> m_sim_buffer_{ PPS_Settings::particle_count }; // sim -> render (lock-free)

	// render → sim  (low frequency, mutex protected)
	std::mutex             m_cmd_mutex{};
	std::queue<SimCommand> m_commands{};

	std::thread m_sim_thread_;
	std::atomic<bool> running{ true }; // replace your existing bool if you have one


public:
	Simulation(); //, sf::Style::None)
	void init_imGUI();
	void run();

	void quit();
	void update();
	void resolve_modifications();
	void handle_events();
	void dispatch_event(const sf::Event& event, const sf::Vector2f& cam_pos);
	void handle_keyboard_events(const sf::Keyboard::Key& event_key_code);
	void handle_pause_toggle();
	void handle_mouse_press(const sf::Vector2f& cam_pos);
	void handle_mouse_release();

private:
	void render();
	void manage_frame_rate();
	void handle_imGUI(const SimSnapshot& snap, float dt);
	void draw_tab(const SimSnapshot& snap, float dt);
};
