#include "particle_system.h"
#include "../simulation/utils.h"


void ParticleSystem::init_particles()
{

	particles.resize(particle_count, Particle());

	unsigned id = 0;
	const float bx = 0.f;
	const float by = 0.f;
	const sf::Rect<float> bounds = { bx,by, screen_bounds.width - bx*2, screen_bounds.height - by*2 };
	for (Particle& particle : particles)
	{
		particle.id = id++;
		particle.position = RandomDist::randPosInRect(bounds);
		particle.angle_radians = RandomDist::randRange(0.f, 2 * pi);
	}

}


void ParticleSystem::update()
{
	
	update_hash_grid();

	for (Particle& particle : particles)
	{
		update_particle_locals(particle);
		update_particle_positioning(particle);
	}
}


void ParticleSystem::update_hash_grid()
{
	spatial_hash_grid.clear();
	for (const Particle& particle : particles)
	{
		spatial_hash_grid.addObject(particle.position, particle.id);
	}
}


void ParticleSystem::reset_particle(Particle& particle) const
{
	particle.left = 0;
	particle.right = 0;
	particle.near_inner = 0;
	particle.sin_angle = std::sin(particle.angle_radians);
	particle.cos_angle = std::cos(particle.angle_radians);
}


void ParticleSystem::update_particle_locals(Particle& particle)
{
	reset_particle(particle);

	const c_Vec& near = spatial_hash_grid.find(particle.position);
	for (unsigned i = 0; i < near.size; i++)
	{
		Particle& other_particle = particles[near.array[i]];

		const float dist_sq = wrappedDistSq(particle.position, other_particle.position, screen_bounds.width, screen_bounds.height);
		constexpr float rad_sq = visual_radius * visual_radius;

		if (dist_sq > 0 && dist_sq < rad_sq)
		{
			const bool right = isOnRightHemisphere(particle.position, particle.angle_radians, other_particle.position, screen_bounds.width, screen_bounds.left);

			if (right) ++particle.right;
			else       ++particle.left;

			if (dist_sq < magenta_radius * magenta_radius)
				++particle.near_inner;
		}
	}
}

void ParticleSystem::update_particle_positioning(Particle& ptk) const
{
	// delta_phi = alpha + beta × N × sign(R - L)
	const unsigned sum = ptk.right + ptk.left;
	const float delta = alpha + beta * sum * sign(ptk.right - ptk.left);
	ptk.angle_radians += delta * (pi / 180.f);

	float deltaX = gamma * std::cos(ptk.angle_radians);
	float deltaY = gamma * std::sin(ptk.angle_radians);
	ptk.position += {deltaX, deltaY};

	clamp_angle(ptk.angle_radians);

	border(ptk.position, screen_bounds);
}



void ParticleSystem::draw_hash_grid(sf::RenderWindow& window)
{
	window.draw(spatial_hash_grid.vertexBuffer);
}


sf::Color ParticleSystem::get_color(const Particle& particle)
{
	if (particle.near_inner > colors[colors.size() - 1].first)
	{
		return colors[colors.size() - 1].second;
	}

	for (int i = colors.size() - 2; i >= 0; --i)
	{
		if (particle.left + particle.right >= colors[i].first)
		{
			return colors[i].second;
		}
	}

	return { 0, 0,0 };
}

void ParticleSystem::render_particles(sf::RenderWindow& window)
{
	sf::CircleShape renderer{};
	renderer.setRadius(radius);
	renderer.setFillColor({ 255, 255, 255 });

	for (Particle& particle : particles)
	{
		renderer.setFillColor(get_color(particle));

		renderer.setPosition(particle.position - sf::Vector2f{ radius, radius });
		window.draw(renderer, sf::BlendAdd);
	}
}


void ParticleSystem::debug_particles(sf::RenderWindow& window) const
{
	sf::CircleShape p_visual_radius{visual_radius};
	p_visual_radius.setFillColor({ 0, 0, 0, 0 });
	p_visual_radius.setOutlineThickness(1);

	for (unsigned i = 0; i < particles.size(); i++)
	{
		if (i == 0)
		{
			p_visual_radius.setOutlineColor({ 0, 255 ,0 });
		}

		else
		{
			p_visual_radius.setOutlineColor({ 255 ,255, 255 });
		}

		if (i % 1 == 0)
		{
			p_visual_radius.setPosition(particles[i].position - sf::Vector2f{visual_radius, visual_radius});
			window.draw(p_visual_radius);

			window.draw(createLine(particles[i].position, particles[i].angle_radians, 15));
		}
	}
}
