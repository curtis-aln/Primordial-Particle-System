#include "simulation.h"

#include "imgui-SFML.h"
#include "imgui.h"
#include "implot.h"

Simulation::Simulation() : window_(sf::VideoMode({ screen_width, screen_height }), simulation_title)//, sf::Style::None)
{
	window_.setFramerateLimit(max_frame_rate);
	window_.setVerticalSyncEnabled(Vsync);
	window_.setFramerateLimit(144);

	if (set_Random_seed)
	{
		Random::set_seed(0);
	}

	init_imGUI();

	// Translating the camera to be in the middle of the screen and zooming it in
	camera.m_view_.move({ world_width / 2.f, world_height / 2.f });

	const float zoom_factor = 1 / (particle_radius * 200.f);
	// todo
}

void Simulation::init_imGUI()
{
	if (!ImGui::SFML::Init(window_))
		std::cerr << "[ERROR]: Failed to initialize ImGui-SFML\n";

	constexpr int ui_scale_percent = 100.f;
	ImGui::GetIO().FontGlobalScale = ui_scale_percent / 100.f;

	ImPlot::CreateContext();
}


void Simulation::run()
{
	m_sim_thread_ = std::thread([this]
		{
			while (running)
				update();
		});

	while (running)
	{
		handle_events();
		manage_frame_rate();
		render();
	}

	m_sim_thread_.join();
	ImGui::SFML::Shutdown();
	ImPlot::DestroyContext();
}

void Simulation::quit()
{
	running_ = false;
}

void Simulation::update()
{
	resolve_modifications();

	if (particle_system_.iterations_ % add_to_grid_freq == 0)
	{
		particle_system_.add_particles_to_grid();
	}
	particle_system_.update_particles();

	// Package results into the triple buffer
	SimSnapshot& snap = m_sim_buffer_.get_write_buffer();
	//pps_renderer_.notify_new_snapshot(snap);  // resets the age clock

	// Filling the snapshot with information
	particle_system_.fill_snapshot(snap);
	snap.stats.fps = fps_;
	snap.stats.m_total_time_elapsed_ = m_total_time_elapsed_;

	m_sim_buffer_.publish();
}

void Simulation::render()
{
	if (!m_sim_buffer_.has_published()) return; // wait for first frame

	const bool is_new_frame = m_sim_buffer_.has_new_frame();
	const SimSnapshot& snap = m_sim_buffer_.begin_read();

	if (is_new_frame)
		pps_renderer_.notify_new_snapshot(snap);

	float dt = static_cast<float>(m_delta_time_.get_delta());
	m_total_time_elapsed_ += dt;

	const sf::Vector2f mouse_pos = camera.get_world_mouse_pos();

	window_.clear(screen_color);

	pps_renderer_.render(snap, camera, false);

	if (debug_)
	{
		pps_renderer_.render_debug(snap, mouse_pos, debug_radius);
		particle_system_.beacons.render(window_);
	}

	handle_imGUI(snap, dt);

	m_sim_buffer_.end_read();

	ImGui::SFML::Render(window_);
	window_.display();
}

void Simulation::manage_frame_rate()
{
	fps_ = static_cast<float>(clock_.get_average_frame_rate());
	clock_.update_frame_rate();

	std::ostringstream title;
	title << "Primordial Particle System"
		<< " | FPS: " << std::fixed << std::setprecision(1) << fps_;

	window_.setTitle(title.str());
}