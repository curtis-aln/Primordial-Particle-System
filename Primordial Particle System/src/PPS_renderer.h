#pragma once


inline sf::Color get_color(const uint16_t total)
{
	// Assuming colors is sorted in descending order of first element
	auto it = std::lower_bound(SystemSettings::colors.rbegin(), SystemSettings::colors.rend(), total,
		[](const auto& pair, uint16_t value) { return pair.first > value; });
	return it != SystemSettings::colors.rend() ? it->second : SystemSettings::colors.back().second;
}

class PPS_Renderer
{
private:
	sf::VertexArray vertex_array;
	sf::Shader particle_shader;

	// Debug rendering
	sf::RectangleShape visual_radius_shape;
	sf::VertexArray debug_lines;
	Font& debug_font;

	void initializeShader()
	{
		if (!particle_shader.loadFromMemory(
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
	PPS_Renderer(size_t population_size, float particle_size, Font& font) : debug_font(font)
	{
		vertex_array.setPrimitiveType(sf::Quads);
		vertex_array.resize(population_size * 4);

		initializeShader();

		visual_radius_shape.setFillColor(sf::Color::Transparent);
		visual_radius_shape.setOutlineThickness(1);
		visual_radius_shape.setOutlineColor(sf::Color(255, 255, 255, 100));

		debug_lines.setPrimitiveType(sf::Lines);
	}

	void updateParticles(const std::array<sf::Vector2f, SystemSettings::particle_count>& positions,
		const std::array<uint16_t, SystemSettings::particle_count>& neighbourhood_count,
		float particle_size)
	{
		for (size_t i = 0; i < positions.size(); ++i) 
		{
			const sf::Vector2f& center = positions[i];
			const sf::Color color = get_color(neighbourhood_count[i]);
			const float half_size = particle_size / 2.f;

			sf::Vertex* quad = &vertex_array[i * 4];
			quad[0].position = center + sf::Vector2f(-half_size, -half_size);
			quad[1].position = center + sf::Vector2f(half_size, -half_size);
			quad[2].position = center + sf::Vector2f(half_size, half_size);
			quad[3].position = center + sf::Vector2f(-half_size, half_size);

			quad[0].color = quad[1].color = quad[2].color = quad[3].color = color;
		}
	}

	void render(sf::RenderWindow& window)
	{
		sf::RenderStates states;
		states.shader = &particle_shader;
		particle_shader.setUniform("resolution", sf::Glsl::Vec2(window.getSize()));

		window.draw(vertex_array, states);
	}


	void renderDebug(sf::RenderWindow& window,
		const std::array<sf::Vector2f, SystemSettings::particle_count>& positions,
		const std::array<float, SystemSettings::particle_count>& angles,
		const std::array<uint16_t, SystemSettings::particle_count>& neighbourhood_count,
		float visual_radius, float particle_size)
	{
		debug_lines.clear();
		debug_lines.resize(positions.size() * 2);

		for (size_t i = 0; i < positions.size(); ++i) 
		{
			const sf::Vector2f& position = positions[i];
			float angle = angles[i];

			// Draw direction line
			sf::Vector2f direction = sf::Vector2f(std::sin(angle), std::cos(angle)) * particle_size;
			debug_lines[i * 2].position = position;
			debug_lines[i * 2].color = sf::Color::White;
			debug_lines[i * 2 + 1].position = position + direction;
			debug_lines[i * 2 + 1].color = sf::Color::White;

			// Draw visual radius
			visual_radius_shape.setSize(sf::Vector2f(visual_radius * 2, visual_radius * 2));
			visual_radius_shape.setPosition(position - sf::Vector2f(visual_radius, visual_radius));
			window.draw(visual_radius_shape, sf::BlendAdd);

			// Draw debug text
			sf::Vector2f text_pos = position + sf::Vector2f(0, 30);
			debug_font.draw(text_pos, "nearby: " + std::to_string(neighbourhood_count[i]), true);
		}

		window.draw(debug_lines);
	}
};
