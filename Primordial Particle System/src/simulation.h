#pragma once
#include "settings.h"
#include "particle_system/particle_system.h"
#include "utils/smooth_frame_rates.h"
#include "utils/font.h"
#include "utils/Camera.hpp"
#include "utils/SFML_grid.h"

#include <string>

#include "../IMGUI/imgui.h"
#include "../IMGUI/imgui-SFML.h"

inline float interpolateZoom(float currentZoom, float targetZoom, float deltaTime, float speed = 5.0f)
{
	// Smooth interpolation
	return currentZoom + (targetZoom - currentZoom) * (speed * deltaTime);
}

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
	size_t iterations_ = 0;

	bool paused_ = true;
	bool running_ = true;
	bool render_hash_grid_ = false;
	bool debug_ = false;
	bool rendering_ = true;

	// display mode makes the particle movement less jittery by substepping, it makes the organisms displayed circular,
	// and the colours more vibrant and bloomish.
	bool display_mode_ = false;

	// radius around the mouse in which debug settings are shown
	float debug_radius = 8000.f;
	const float change_in_debug_radius = 500.f;
	SFML_Grid grid{ window_, sf::FloatRect(0, 0, world_width, world_height), 10 };

	// The particle system
	ParticlePopulation<particle_count> particle_system_{ window_ };

	sf::Clock delta_clock{}; // for imgui


public:
	Simulation() : window_(
		sf::VideoMode(screen_width, screen_height),
		simulation_title,
		sf::Style::Default,
		sf::ContextSettings{ 0, 0, 8 } // Anti-aliasing level set to 8
	)
	{
		ImGui::SFML::Init(window_);

		window_.setFramerateLimit(max_frame_rate);
		window_.setVerticalSyncEnabled(Vsync);

		// setting the camera pos to the center by defualt
		camera.set_camera_position({ world_width / 2, world_height / 2 });
		camera.update(0.f);
	}

	void run()
	{
		while (running_)
		{
			poll_events();
			process_imgui();

			for (size_t i = 0; i < sub_iterations; ++i)
			{
				if (iterations_ % add_to_grid_freq == 0)
				{
					particle_system_.add_particles_to_grid();
				}
				particle_system_.update_particles(paused_);
				++iterations_;
			}

			window_.clear(screen_color);
			if (rendering_)
			{
				render();
			}
			ImGui::SFML::Render(window_);
			window_.display();
			
			update_caption();

			iterations_ += sub_iterations;
		}

		ImGui::SFML::Shutdown();
	}

	void quit()
	{
		running_ = false;
	}

private:
	void render()
	{
		const sf::Vector2f mouse_pos = camera.get_world_mouse_pos();


		particle_system_.render(window_, render_hash_grid_, mouse_pos);

		if (debug_)
		{
			particle_system_.render_debug(window_, mouse_pos, debug_radius);
			particle_system_.beacons.render(window_);
		}

		//grid.draw();
	}

	void process_imgui()
	{
		// Start a new ImGui frame
		ImGui::SFML::Update(window_, delta_clock.restart());
		
		imgui_update_rules();
		imgui_color_picker();
		
	}

	void imgui_update_rules()
	{
		ImGui::Begin("Update Rules");

		// sliders
		const char* one_dp = "%.0f";
		const float range = 180;
		ImGui::SliderFloat("Alpha", &UpdateRules::alpha, -range, range, one_dp);
		ImGui::SliderFloat("Beta", &UpdateRules::beta, -range, range, one_dp);

		ImGui::End();
	}

	void imgui_color_picker()
	{
		ImGui::Begin("Color picker");

		const char* one_dp = "%.0f";
		ImGui::SliderFloat("range 1", &ColorSettings::range1, 0, 20, one_dp);
		ImGui::SliderFloat("range 2", &ColorSettings::range2, 0, 40, one_dp);
		ImGui::SliderFloat("range 3", &ColorSettings::range3, 0, 60, one_dp);

		ImGui::ColorEdit3("First Color", ColorSettings::first_color);
		ImGui::ColorEdit3("Second Color", ColorSettings::second_color);
		ImGui::ColorEdit3("Third Color", ColorSettings::third_color);
		ImGui::ColorEdit3("Fourth Color", ColorSettings::fourth_color);

		ImGui::End();
	}

	void key_press_events(const sf::Keyboard::Key& event_key_code)
	{
		switch (event_key_code)
		{
		case sf::Keyboard::Escape:
			quit();
			break;

		case sf::Keyboard::Space:
			paused_ = !paused_;
			break;

		case sf::Keyboard::G:
			render_hash_grid_ = !render_hash_grid_;
			break;

		case sf::Keyboard::R:
			rendering_ = !rendering_;
			break;

		case sf::Keyboard::D:
			debug_ = !debug_;
			break;
		default: ;
		}
	}


	void poll_events()
	{
		float deltaTime = clock_.get_delta_time();
		camera.update(deltaTime);

		sf::Event event{};
		while (window_.pollEvent(event))
		{
			ImGui::SFML::ProcessEvent(event);

			if (event.type == sf::Event::Closed)
				quit();


			else if (event.type == sf::Event::KeyPressed)
			{
				key_press_events(event.key.code);
			}


			else if (event.type == sf::Event::MouseWheelScrolled)
			{
				const float delta = event.mouseWheelScroll.delta;
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
				{
					const int sign = delta < 0 ? -1 : 1;
					debug_radius += change_in_debug_radius * sign;
				}
				else
				{
					camera.zoom(delta);
				}
			}
		}
		
		if (ImGui::GetIO().WantCaptureMouse)
		{
			return;
		}

		if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
		{
			camera.translate();
		}

		else if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
		{
			particle_system_.beacons.add_beacons(camera.get_world_mouse_pos(), debug_radius);
		}
	}


	void update_caption()
	{
		clock_.update_frame_rate();
		const auto fps = static_cast<int>(clock_.get_average_frame_rate());

		const sf::Vector2f start = { 40.f, 40.f };
		constexpr float spacing = 20.f;
		float i = 2;

		title_font.draw(start, simulation_title);
		text_font.draw(start + sf::Vector2f{0.f, spacing * i++}, std::to_string(fps) + " fps");
		text_font.draw(start + sf::Vector2f{0.f, spacing * i++}, "particles");
		text_font.draw(start + sf::Vector2f{0.f, spacing * i}, "iterations");


		window_.setTitle(std::to_string(fps));
	}

};