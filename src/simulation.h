#pragma once
#include "settings.h"
#include "particle_system/particle_system.h"
#include "utils/smooth_frame_rates.h"
#include "utils/font.h"
#include "utils/Camera.hpp"
#include "utils/SFML_grid.h"

#include <string>

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

	// radius around the mouse in which debug settings are shown
	float debug_radius = 8000.f;
	const float change_in_debug_radius = 500.f;
	SFML_Grid grid{ window_, sf::FloatRect({0, 0}, {world_width, world_height}), 10 };

	// The particle system
	ParticlePopulation particle_system_{ window_ };


public:
	Simulation() : window_(sf::VideoMode({screen_width, screen_height}), simulation_title)//, sf::Style::None)
	{
		window_.setFramerateLimit(max_frame_rate);
		window_.setVerticalSyncEnabled(Vsync);
		
		if (set_Random_seed)
		{
			Random::set_seed(0);
		}
	}

	void run()
	{

		while (running_)
		{
			key_press_events();
			camera.update(clock_.get_delta_time());


			for (size_t i = 0; i < sub_iterations; ++i)
			{
				if (iterations_ % add_to_grid_freq == 0)
				{
					particle_system_.add_particles_to_grid();
				}
				particle_system_.update_particles(paused_);
				++iterations_;
			}

			camera.update_window_view();


			if (rendering_)
			{
				render();
			}
			update_caption();

			iterations_ += sub_iterations;
		}
	}

	void quit()
	{
		running_ = false;
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
			particle_system_.beacons.render(window_);
		}

		//grid.draw();
		window_.display();
	}

	void key_press_events()
	{
		while (const std::optional event = window_.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
			{
				quit();
				return;
			}

			if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
			{
				if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
				{
					quit();
					return;
				}

				if (keyPressed->scancode == sf::Keyboard::Scancode::Space)
				{
					paused_ = !paused_;
				}
				else if (keyPressed->scancode == sf::Keyboard::Scancode::G)
				{
					render_hash_grid_ = !render_hash_grid_;
				}
				else if (keyPressed->scancode == sf::Keyboard::Scancode::R)
				{
					rendering_ = !rendering_;
				}
				else if (keyPressed->scancode == sf::Keyboard::Scancode::D)
				{
					debug_ = !debug_;
				}
			}

			else if (const auto* mouseWheelScrolled = event->getIf<sf::Event::MouseWheelScrolled>())
			{
				const float delta = mouseWheelScrolled->delta;
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

			// if the mouse is pressed we begin pan, when it is released we end pan. this allows for smooth movement rather than snapping to the mouse position
			else if (event->is<sf::Event::MouseButtonPressed>())
			{
				if (event->getIf<sf::Event::MouseButtonPressed>()->button == sf::Mouse::Button::Left)
				{
					camera.begin_pan();
				}
				else if (event->getIf<sf::Event::MouseButtonPressed>()->button == sf::Mouse::Button::Right)
				{
					particle_system_.beacons.add_beacons(camera.get_world_mouse_pos(), debug_radius);
				}
			}
			else if (event->is<sf::Event::MouseButtonReleased>())
			{
				if (event->getIf<sf::Event::MouseButtonReleased>()->button == sf::Mouse::Button::Left)
				{
					camera.end_pan();
				}
			}
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