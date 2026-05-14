#pragma once
#include <SFML/Graphics.hpp>



inline float dist_squared(const sf::Vector2f position_a, const sf::Vector2f position_b)
{
	const sf::Vector2f delta = position_b - position_a;
	return delta.x * delta.x + delta.y * delta.y;
}


inline void draw_thick_line(sf::RenderWindow& window, const sf::Vector2f& point, const float length, const float angle,
	const float thickness = {}, const sf::Color& fill_color = { 255, 255, 255 })
{

	// Create the rectangle shape
	sf::RectangleShape line(sf::Vector2f(length, thickness));
	line.setOrigin(0, thickness / 2.0f);
	line.setPosition(point);
	line.setRotation(angle);
	line.setFillColor(fill_color);

	// Draw the line
	window.draw(line);
}


inline sf::VertexArray createLine(const sf::Vector2f& position, const float angleRadians, const float length)
{
	sf::VertexArray line(sf::Lines, 2);

	// Calculate the endpoint of the line
	const float endX = position.x + length * std::cos(angleRadians);
	const float endY = position.y + length * std::sin(angleRadians);
	
	// Set the vertices of the line segment
	line[0].position = position;
	line[1].position = sf::Vector2f(endX, endY);
	line[0].color = { 0, 0, 255 };
	line[1].color = { 0, 0, 255 };

	return line;
}

inline sf::VertexArray createLine(const sf::Vector2f& pos1, const sf::Vector2f& pos2)
{
	sf::VertexArray line(sf::Lines, 2);

	// Set the vertices of the line segment
	line[0].position = pos1;
	line[1].position = pos2;
	line[0].color = { 0, 255, 0 };
	line[1].color = { 0, 255, 0 };

	return line;
}


inline sf::Color convertToSFColor(const float color[3])
{
	return sf::Color(
		static_cast<sf::Uint8>(color[0] * 255), // Red component
		static_cast<sf::Uint8>(color[1] * 255), // Green component
		static_cast<sf::Uint8>(color[2] * 255)  // Blue component
	);
}

// turning a nearby count into a color which is smoothed
inline sf::Color interpolate(const float color1[3], const float color2[3], float t)
{
	t = std::clamp(t, 0.0f, 1.0f); // Clamp t to the range [0, 1]

	float interpolated[3] = {
		color1[0] + t * (color2[0] - color1[0]),
		color1[1] + t * (color2[1] - color1[1]),
		color1[2] + t * (color2[2] - color1[2])
	};

	return sf::Color(
		static_cast<sf::Uint8>(interpolated[0] * 255),
		static_cast<sf::Uint8>(interpolated[1] * 255),
		static_cast<sf::Uint8>(interpolated[2] * 255)
	);
}

