#pragma once

#include <SFML/Graphics.hpp>
#include "../settings.h"
#include "../spatial_hash_grid.h"
#include "../rand_dist.h"

struct Particle
{
	unsigned id{};

	sf::Vector2f position{};
	float angle_radians{};

	uint8_t left;
	uint8_t right;
	uint8_t near_inner;

	float sin_angle;
	float cos_angle;
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
	ParticleSystem(const sf::Rect<float>& screen_size)
	: spatial_hash_grid(screen_size, spatial_hash_dims),
	  screen_bounds( 0.f, 0.f, static_cast<float>(screen_size.width), static_cast<float>(screen_size.height))
	{
		init_particles();
	}

	void update();
	void draw_hash_grid(sf::RenderWindow& window);
	void render_particles(sf::RenderWindow& window);
	void debug_particles(sf::RenderWindow& window) const;

private:
	std::vector<Particle> particles{};
	SpatialHashGrid spatial_hash_grid{};

	sf::Rect<float> screen_bounds{};
	
};