#pragma once

#include <SFML/Graphics.hpp>
#include "../settings.h"
#include "../utils/spatial_hash_grid.h"
#include "../utils/random.h"

struct Particle
{
	inline static sf::Vector2f a_range{ 180.f, 180.f };
	inline static sf::Vector2f b_range{ -1.f, 17.f };

	float alpha = 180;
	float beta = 17;
	float delta = 0.1f;

	unsigned id{};

	sf::Vector2f position{};
	float angle_radians{};

	uint8_t left;
	uint8_t right;
	uint8_t near_inner;


	Particle()
	{
		
	}
};



class ParticleSystem : SystemSettings
{
	void init_particles();
	void update_hash_grid();
	void reset_particle(Particle& particle) const;
	void update_particle_locals(Particle& particle);
	void update_particle_positioning(Particle& ptk) const;
	static sf::Color get_color(const Particle& particle);


public:
	ParticleSystem(const sf::FloatRect& screen_size)
	: spatial_hash_grid(screen_size, spatial_hash_dims),
	  screen_bounds(screen_size)
	{
		init_particles();
	}

	void update();
	void draw_hash_grid(sf::RenderWindow& window) const;
	void render_particles(sf::RenderWindow& window, const bool mode);
	void debug_particles(sf::RenderWindow& window) const;

public:
	std::vector<Particle> particles{};
	SpatialHashGrid spatial_hash_grid{};

	sf::FloatRect screen_bounds{};
	
};