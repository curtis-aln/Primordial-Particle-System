#pragma once
#include "../settings.h"
#include "../spatial_hash_grid.h"
#include "../particle system/particle_system.h"


class Simulation : SystemSettings, SimulationSettings
{
	void render();
	void keyPressEvents(const sf::Keyboard::Key& event_key_code);
	void pollEvents();
	void update_caption();

public:
	Simulation();

	void run();


private:
	ParticleSystem particle_system{ screen_bounds };

	sf::RenderWindow window{};
	sf::Clock clock{};

	bool paused = false;
	bool running = true;
	bool render_hash_grid = false;
};