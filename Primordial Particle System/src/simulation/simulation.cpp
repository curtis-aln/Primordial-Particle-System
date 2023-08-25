#include "simulation.h"
#include <string>
#include <sstream>
#include "../rand_dist.h"

Simulation::Simulation()
	: window(sf::VideoMode(screen_width, screen_height), window_name)
{
	window.setFramerateLimit(frame_rate);
	window.setVerticalSyncEnabled(false);
}



void Simulation::run()
{
	while (running)
	{
		pollEvents();

		window.clear(screen_color);

		if (!paused)
		{
			particle_system.update();
		}

		render();

		update_caption();
	}
}


void Simulation::render()
{

	particle_system.render_particles(window);
	//particle_system.debug_particles(window);

	if (render_hash_grid)
	{
		particle_system.draw_hash_grid(window);
	}

	window.display();
}



void Simulation::keyPressEvents(const sf::Keyboard::Key& event_key_code)
{
	switch (event_key_code)
	{
	case sf::Keyboard::Escape:
		running = false;
		break;

	case sf::Keyboard::Space:
		paused = not paused;
		break;

	case sf::Keyboard::R:
		render_hash_grid = not render_hash_grid;
		break;
	}
}


void Simulation::pollEvents()
{
	sf::Event event{};
	while (window.pollEvent(event))
	{
		if (event.type == sf::Event::Closed)
			running = false;

		else if (event.type == sf::Event::KeyPressed)
			keyPressEvents(event.key.code);
	}
}


void Simulation::update_caption()
{
	
	const sf::Int32 msPerFrame = 1 / clock.restart().asSeconds();

	std::ostringstream oss;
	oss << window_name << " " << msPerFrame << " fps" << "\n";
	const std::string stringFrameRate = oss.str();
	window.setTitle(stringFrameRate);
	
}