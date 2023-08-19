#pragma once
#include "settings.h"

struct Particle
{
	unsigned id{};
	sf::Vector2f position{};
	float angle{};

	sf::Vector2u nearby{};
	sf::Vector2f sincosine{};
};


class Simulation : SystemSettings, SimulationSettings
{
	void init_particles();
	void render();
	void render_particles();
	void keyPressEvents(const sf::Keyboard::Key& event_key_code);
	void pollEvents();
	void update_caption();

public:
	Simulation();

	void run();
	void update();
	void update_particle_locals(Particle& particle);
	void update_particle_positioning(Particle& particle);

private:
	std::vector<Particle> particles{};

	sf::RenderWindow window{};
	sf::Clock clock{};

	sf::Rect<float> bounds = { 0.f, 0.f, static_cast<float>(screen_width), static_cast<float>(screen_height) };

	bool paused = false;
	bool running = true;
};