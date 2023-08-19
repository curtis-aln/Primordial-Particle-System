#include "simulation.h"
#include <string>
#include <sstream>
#include "../rand_dist.h"
#include <cmath>
#include "utils.h"

Simulation::Simulation()
	: window(sf::VideoMode(screen_width, screen_height), window_name)
{
	window.setFramerateLimit(frame_rate);
	window.setVerticalSyncEnabled(false);

	init_particles();
}


void Simulation::init_particles()
{

	particles.resize(particle_count, Particle());

	unsigned id = 0;
	for (Particle& particle : particles)
	{
		particle.id = id++;
		particle.position = RandomDist::randPosInRect(bounds);
		particle.angle = RandomDist::randRange(0.f, 360.f);
	}

	const sf::Vector2f center = { screen_width / 2, screen_height / 2 };
	particles[0].position = { center.x - 20, center.y };
	particles[1].position = { center.x + 20, center.y };
}


void Simulation::run()
{
	while (running)
	{
		pollEvents();

		update();
		render();

		update_caption();
	}
}


void Simulation::update()
{
	if (!paused)
	{
		for (Particle& particle : particles)
		{
			update_particle_locals(particle);
		}

		for (Particle& particle : particles)
		{
			update_particle_positioning(particle);
		}
	}
}


void Simulation::update_particle_locals(Particle& particle)
{
	particle.nearby = {0, 0};

	for (Particle& other_particle : particles)
	{
		const float dist_sq = wrappedDistSq(particle.position, other_particle.position, screen_width, screen_height);
		const float rad_sq = visual_radius * visual_radius;

		if (dist_sq > 0 && dist_sq < rad_sq)
		{
			if (isOnRightHemisphere(particle.position, particle.angle, other_particle.position, screen_width, screen_height))
			{
				particle.nearby.y += 1;
			}
			else
			{
				particle.nearby.x += 1;
			}
		}
	}
}

void Simulation::update_particle_positioning(Particle& particle)
{
	// delta_phi = alpha + beta × N × sign(R - L)
	unsigned x = particle.nearby.x, y = particle.nearby.y;
	unsigned N = x + y;
	const float delta = alpha + beta * N * sign(y - x);
	particle.angle += delta;
	clamp_angle(particle.angle);

	const float angleRadians = particle.angle * (pi / 180.f);
	float deltaX = gamma * std::cos(angleRadians);
	float deltaY = gamma * std::sin(angleRadians);
	particle.position += {deltaX, deltaY};

	border(particle.position, bounds);
}


void Simulation::render()
{
	window.clear(screen_color);

	render_particles();

	window.display();
}

void Simulation::render_particles()
{
	sf::CircleShape renderer{};
	renderer.setRadius(radius);
	renderer.setFillColor({ 255, 255, 255 });

	sf::CircleShape v_radius{};
	v_radius.setRadius(visual_radius);
	v_radius.setFillColor({ 0, 0, 0, 0 });
	v_radius.setOutlineThickness(1);
	v_radius.setOutlineColor({ 255, 255, 255 });

	unsigned i = 0;
	for (Particle& particle : particles)
	{
		if (i++ == 0)
		{
			renderer.setFillColor({ 0, 255, 0 });
			v_radius.setPosition(particle.position - sf::Vector2f{ visual_radius, visual_radius });
			window.draw(v_radius);

		}
		else
		{
			renderer.setFillColor({ 255, 255, 255 });
		}

		renderer.setPosition(particle.position - sf::Vector2f{radius, radius});
		window.draw(renderer);

		window.draw(createLine(particle.position, particle.angle, radius));

	}
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