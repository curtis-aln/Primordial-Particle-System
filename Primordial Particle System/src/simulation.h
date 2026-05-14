#pragma once
#include "settings.h"
#include "particle_system.h"
#include "utils/smooth_frame_rates.h"
#include "utils/font.h"
#include "utils/Camera.hpp"
#include "utils/Timelapse.h"

#include <string>

class Simulation : PPS_Settings, SimulationSettings
{
	// SFML
	sf::RenderWindow window_{};

	// Smooths Frame rates by averaging them
	FrameRateSmoothing<10> clock_{};

	// Allows for translation & Zooming
	Camera camera{ &window_, 1.f / scale_factor };

	// Recording
	Timelapse timelapse_{ window_, 0.5f };

	// Two separate font sizes. allows rendering of text on-screen
	Font title_font = { &window_, 60, "fonts/Calibri.ttf" };
	Font text_font = { &window_, 35, "fonts/Calibri.ttf" };

	// Runtime variables and statistics
	size_t iterations_ = 0;

	bool paused_ = false;
	bool running_ = true;
	bool render_hash_grid_ = false;
	bool debug_ = false;
	bool rendering_ = true;

	// radius around the mouse in which debug settings are shown
	float debug_radius = 8000.f;
	const float change_in_debug_radius = 500.f;

	// The particle system
	ParticlePopulation<particle_count> particle_system_{ window_ };


public:
	Simulation() : window_(sf::VideoMode(screen_width, screen_height), simulation_title)//, sf::Style::None)
	{
		window_.setFramerateLimit(max_frame_rate);
		window_.setVerticalSyncEnabled(Vsync);
	}

	void run()
	{

		while (running_)
		{
			poll_events();

			for (size_t i = 0; i < sub_iterations; ++i)
			{
				if (iterations_ % add_to_grid_freq == 0)
				{
					particle_system_.add_particles_to_grid();
				}
				particle_system_.update_particles(paused_);
				++iterations_;
			}

			if (rendering_)
			{
				render();

				if (record)
				{
					timelapse_.capture();
				}
			}
			update_caption();

			iterations_ += sub_iterations;
		}
	}

	void quit()
	{
		if (record)
		{
			timelapse_.save_video("TimeLapse");
		}
		running_ = false;

		//int max = 0;
		//int min = 0;
		//float sum = 0.f;
		//std::vector<int>& times = particle_system_.times_record;
		//for (int i = 0; i < times.size(); ++i)
		//{
		//	const int time = times[i];
		//	sum += static_cast<float>(time);
		//
		//	if (time > max)
		//	{
		//		max = time;
		//	}
		//
		//	if (time < min)
		//	{
		//		min = time;
		//	}
		//}
		//sum /= times.size();
		//
		//std::cout << "average frames per cell: " << sum << "\n";
		//std::cout << "max frames in cell: " << max << "\n";
		//std::cout << "min frames in cell: " << min << "\n";
	}

private:
	void render()
	{
		const sf::Vector2f mouse_pos = camera.get_world_mouse_pos();

		window_.clear(screen_color);
		particle_system_.render(window_, render_hash_grid_, mouse_pos);

		if (debug_)
		{
			particle_system_.render_debug(window_, mouse_pos, debug_radius);
		}
		window_.display();
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
		camera.update();

		sf::Event event{};
		while (window_.pollEvent(event))
		{
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
				//else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
				//{
					//const int sign = delta < 0 ? -1 : 1;
					//decay += change_in_decay * sign;
				//}
				else
				{
					camera.zoom(delta);
				}
			}
		}

		if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
		{
			camera.translate();
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