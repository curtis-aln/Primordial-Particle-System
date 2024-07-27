#pragma once
#include "settings.h"
#include "utils/utils.h"
#include "utils/font.h"


sf::Color getColor(float density)
{
	float range1 = 22;
	float range2 = 34;
	float range3 = 40;

	const sf::Uint8 a = 250;
	const sf::Color Red = { 255, 0, 0, a };
	const sf::Color Green = { 50, 255, 0, a };
	const sf::Color Blue = { 0, 0, 255, a };
	const sf::Color Magenta = { 255, 0, 255, a };
	const sf::Color Yellow = { 255, 255, 0, a };
	const sf::Color Pink = { 255, 192, 203, a }; 

	sf::Color first_color = Green;
	sf::Color second_color = Blue;
	sf::Color third_color = Pink;
	sf::Color fourth_color = Red;

	if (density <= 0.0f) return first_color;
	else if (density <= range1)
	{
		float factor = density / range1;
		return sf::Color(
			static_cast<sf::Uint8>(first_color.r + factor * (second_color.r - first_color.r)),
			static_cast<sf::Uint8>(first_color.g + factor * (second_color.g - first_color.g)),
			static_cast<sf::Uint8>(first_color.b + factor * (second_color.b - first_color.b))
		);
	}
	else if (density <= range2)
	{
		float factor = (density - range1) / (range2 - range1);
		return sf::Color(
			static_cast<sf::Uint8>(second_color.r + factor * (third_color.r - second_color.r)),
			static_cast<sf::Uint8>(second_color.g + factor * (third_color.g - second_color.g)),
			static_cast<sf::Uint8>(second_color.b + factor * (third_color.b - second_color.b))
		);
	}
	else if (density < range3)
	{
		float factor = (density - range2) / (range3 - range2);
		return sf::Color(
			static_cast<sf::Uint8>(third_color.r + factor * (fourth_color.r - third_color.r)),
			static_cast<sf::Uint8>(third_color.g + factor * (fourth_color.g - third_color.g)),
			static_cast<sf::Uint8>(third_color.b + factor * (fourth_color.b - third_color.b))
		);
	}
	else return fourth_color;
}




class PPS_Renderer : PPS_Graphics
{
private:
	sf::VertexArray vertex_array_;
	sf::Shader particle_shader_;

	// Debug rendering
	sf::CircleShape visual_radius_shape_;
	sf::VertexArray debug_lines_;

	sf::RenderWindow& window_;
	Font debug_font_;

	void initializeShader()
	{
		if (!particle_shader_.loadFromMemory(
			"uniform vec2 resolution;"
			"void main() {"
			"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
			"    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;"
			"    gl_FrontColor = gl_Color;"
			"}",
			"uniform vec2 resolution;"
			"void main() {"
			"    vec2 coord = gl_FragCoord.xy / resolution - vec2(0.5);"
			"    if (abs(coord.x) > 0.55 || abs(coord.y) > 0.55) discard;"
			"    gl_FragColor = gl_Color;"
			"}"
		)) {
			throw std::runtime_error("Failed to load particle shader");
		}
	}

public:
	PPS_Renderer(const size_t population_size, sf::RenderWindow& window)
		: window_(window), debug_font_(&window, 80, "fonts/Calibri.ttf")
	{
		vertex_array_.setPrimitiveType(sf::Quads);
		vertex_array_.resize(population_size * 4);

		initializeShader();

		visual_radius_shape_.setFillColor(sf::Color::Transparent);
		visual_radius_shape_.setOutlineThickness(5);
		visual_radius_shape_.setOutlineColor(sf::Color(255, 255, 255, 100));
		visual_radius_shape_.setRadius(PPS_Settings::visual_radius);

		debug_lines_.setPrimitiveType(sf::Lines);
	}

	void updateParticles(
		const std::vector<float>& positions_x_, 
		const std::vector<float>& positions_y_,
		const std::vector<uint16_t>& neighbourhood_count)
	{
		for (size_t i = 0; i < positions_x_.size(); ++i) 
		{
			const sf::Vector2f center = { positions_x_[i], positions_y_[i] };
			const sf::Color color = getColor(neighbourhood_count[i]);
			const float half_size = particle_radius / 2.f;

			sf::Vertex* quad = &vertex_array_[i * 4];
			quad[0].position = center + sf::Vector2f(-half_size, -half_size);
			quad[1].position = center + sf::Vector2f(half_size, -half_size);
			quad[2].position = center + sf::Vector2f(half_size, half_size);
			quad[3].position = center + sf::Vector2f(-half_size, half_size);

			quad[0].color = quad[1].color = quad[2].color = quad[3].color = color;
		}
	}

	void render()
	{
		sf::RenderStates states;
		states.shader = &particle_shader_;
		particle_shader_.setUniform("resolution", sf::Glsl::Vec2(window_.getSize()));

		window_.draw(vertex_array_, states);
	}


	void render_debug(
		const std::vector<float>& positions_x, 
		const std::vector<float>& positions_y, 
		const std::vector<float>& angles,
		const std::vector<uint16_t>& neighbourhood_count, 
		const sf::Vector2f mouse_pos, 
		const float mouse_radius)
	{
		constexpr float visual_radius = PPS_Settings::visual_radius;

		debug_lines_.clear();
		debug_lines_.resize(positions_x.size() * 2);

		for (size_t i = 0; i < positions_x.size(); ++i) 
		{
			const sf::Vector2f& position = { positions_x[i], positions_y[i] };

			if (dist_squared(position, mouse_pos) > mouse_radius * mouse_radius)
			{
				continue;
			}

			const float angle = angles[i];

			// Draw direction line
			sf::Vector2f direction = sf::Vector2f(std::sin(angle), std::cos(angle)) * particle_radius;
			debug_lines_[i * 2].position = position;
			debug_lines_[i * 2].color = sf::Color::White;
			debug_lines_[i * 2 + 1].position = position + direction;
			debug_lines_[i * 2 + 1].color = sf::Color::White;

			// Draw visual radius
			visual_radius_shape_.setPosition(position - sf::Vector2f(visual_radius, visual_radius));
			window_.draw(visual_radius_shape_, sf::BlendAdd);

			// Draw debug text
			sf::Vector2f text_pos = position + sf::Vector2f(0, particle_radius);
			debug_font_.draw(text_pos, "nearby: " + std::to_string(neighbourhood_count[i]), true);
			debug_font_.draw(text_pos + sf::Vector2f(0, 30), "angle: " + std::to_string(angles[i]), true);
		}

		window_.draw(debug_lines_);
	}
};
