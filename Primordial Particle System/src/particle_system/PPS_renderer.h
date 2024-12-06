#pragma once
#include "../settings.h"
#include "../utils/utils.h"
#include "../utils/font.h"
#include "../utils/spatial_grid.h"
#include "../utils/Camera.hpp"


class PPS_Renderer : ColorSettings
{
private:
	sf::VertexArray vertex_array_{};

	// Debug rendering
	sf::CircleShape visual_radius_shape_;
	sf::VertexArray debug_lines_;

	sf::RenderWindow& window_;
	Font debug_font_;

	// references
	std::vector<float>& positions_x_;
	std::vector<float>& positions_y_;
	std::vector<float>& angles_;
	std::vector<uint16_t>& neighbourhood_count_;
	unsigned verticies_per_particle = 4; // quads

	// colors to be mapped to the ranges
	const sf::Color first_color = Green;
	const sf::Color second_color = Blue;
	const sf::Color third_color = Pink;
	const sf::Color fourth_color = Red;


public:
	PPS_Renderer(sf::RenderWindow& window,
		std::vector<float>& positions_x, std::vector<float>& positions_y,
		std::vector<float>& angles,
		std::vector<uint16_t>& neighbourhood_count)
		: window_(window), debug_font_(&window, FontSettings::font_size_debug, FontSettings::font_path), 
		positions_x_(positions_x), positions_y_(positions_y), angles_(angles),
		neighbourhood_count_(neighbourhood_count)
	{
		
		std::cout << "Renderer Initialized\n";
	}

	void init_vertex_array()
	{
		vertex_array_.setPrimitiveType(sf::Quads);

		vertex_array_.resize(positions_x_.size() * verticies_per_particle);
		std::cout << "renderer vertex array Initialized\n";
	}


	void render_particles()
	{
		for (size_t i = 0; i < positions_x_.size(); ++i) 
		{
			const sf::Vector2f center = { positions_x_[i], positions_y_[i] };

			const sf::Color color = get_color(neighbourhood_count_[i]);
			const float half_size = PPS_Settings::particle_radius / 2.f;

			sf::Vertex* quad = &vertex_array_[i * verticies_per_particle];
			quad[0].position = center + sf::Vector2f(-half_size, -half_size);
			quad[1].position = center + sf::Vector2f(half_size, -half_size);
			quad[2].position = center + sf::Vector2f(half_size, half_size);
			quad[3].position = center + sf::Vector2f(-half_size, half_size);

			quad[0].color = quad[1].color = quad[2].color = quad[3].color = color;
		}

		
		window_.draw(vertex_array_);
	}

	// turning a nearby count into a color which is smoothed
	sf::Color interpolate(const sf::Color& a, const sf::Color& b, float factor) 
	{
		return sf::Color(
			static_cast<sf::Uint8>(a.r + factor * (b.r - a.r)),
			static_cast<sf::Uint8>(a.g + factor * (b.g - a.g)),
			static_cast<sf::Uint8>(a.b + factor * (b.b - a.b))
		);
	}

	sf::Color get_color(const float nearby_count) 
	{
		if (nearby_count <= 0.0f) return first_color;
		if (nearby_count <= range1) return interpolate(first_color, second_color, nearby_count / range1);
		if (nearby_count <= range2) return interpolate(second_color, third_color, (nearby_count - range1) / (range2 - range1));
		if (nearby_count < range3) return interpolate(third_color, fourth_color, (nearby_count - range2) / (range3 - range2));
		return fourth_color;
	}

};
