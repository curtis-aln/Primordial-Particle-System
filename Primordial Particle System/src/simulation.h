#pragma once
#include "settings.h"
#include "particle_system.h"
#include "utils/smooth_frame_rates.h"
#include "utils/font.h"
#include "utils/Camera.hpp"

#include <string>


class Simulation : SystemSettings, SimulationSettings
{
	void render()
	{
		window_.clear(screen_color);
		particle_system_.render(window_, render_hash_grid_);

		if (debug_)
		{
			particle_system_.render_debug(window_, text_font);
		}
		window_.display();
	}

	void key_press_events(const sf::Keyboard::Key& event_key_code)
	{
		switch (event_key_code)
		{
		case sf::Keyboard::Escape:
			running_ = false;
			break;

		case sf::Keyboard::Space:
			paused_ = not paused_;
			break;

		case sf::Keyboard::G:
			render_hash_grid_ = not render_hash_grid_;
			break;

		case sf::Keyboard::R:
			rendering_ = not rendering_;
			break;

		case sf::Keyboard::M:
			mode_ = not mode_;
			break;

		case sf::Keyboard::D:
			debug_ = not debug_;
			break;
		}
	}


	void poll_events()
	{
		camera.update();

		sf::Event event{};
		while (window_.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				running_ = false;


			else if (event.type == sf::Event::KeyPressed)
				key_press_events(event.key.code);


			else if (event.type == sf::Event::MouseWheelScrolled)
				camera.zoom(event.mouseWheelScroll.delta);


			else if (event.type == sf::Event::MouseButtonPressed)
			{
				mouse_pressed_event = true;
			}


			else if (event.type == sf::Event::MouseButtonReleased)
			{
				mouse_pressed_event = false;
			}
		}

		if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
			camera.translate();
	}


	void update_caption()
	{
		fps_manager.update_frame_rate();
		const auto fps = static_cast<int>(fps_manager.get_average_frame_rate());

		const sf::Vector2f start = { 40.f, 40.f };
		constexpr float spacing = 20.f;
		int i = 2;

		title_font.draw(start, window_name);
		text_font.draw(start + sf::Vector2f{0, spacing * i++}, std::to_string(fps) + " fps");
		text_font.draw(start + sf::Vector2f{0, spacing * i++}, "particles");
		text_font.draw(start + sf::Vector2f{0, spacing * i++}, "iterations");


		window_.setTitle(std::to_string(fps));
	}

public:
	Simulation() : window_(sf::VideoMode(screen_width, screen_height), window_name)//, sf::Style::None)
	{
		window_.setFramerateLimit(frame_rate);
		window_.setVerticalSyncEnabled(false);
	}

	void run()
	{
		while (running_)
		{
			poll_events();

			if (!paused_)
			{
				particle_system_.update_vectorized();
			}

			if (rendering_)
			{
				render();
			}
			update_caption();
		}
	}


private:
	ParticlePopulation<particle_count> particle_system_{ };

	sf::RenderWindow window_{};
	sf::Clock clock_{};
	FrameRateSmoothing<10> fps_manager{};

	Camera camera{&window_, 0.1f};

	Font title_font = { &window_, 30, "fonts/Calibri.ttf" };
	Font text_font = { &window_, 20, "fonts/Calibri.ttf" };

	bool paused_ = false;
	bool running_ = true;
	bool mode_ = false;
	bool render_hash_grid_ = false;
	bool debug_ = false;
	bool rendering_ = true;
	bool mouse_pressed_event = false;
};