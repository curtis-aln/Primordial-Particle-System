#pragma once
#include <SFML/Graphics.hpp>

inline static constexpr float pi = 3.141592653589793238462643383279502884197f;
inline static constexpr float two_pi = 2.f * pi;
inline static constexpr float pi_div_180 = pi / 180.f;


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

inline void draw_thick_line(sf::RenderWindow& window, const sf::Vector2f& point1, const sf::Vector2f& point2,
	const float thickness = {}, const sf::Color& fill_color = { 255, 255, 255 })
{
	// Calculate the length and angle of the line
	const float length = std::sqrt(dist_squared(point1, point2));
	const float angle = std::atan2(point2.y - point1.y, point2.x - point1.x) * 180 / pi;

	draw_thick_line(window, point1, length, angle, thickness, fill_color);
}


inline void draw_rect_outline(sf::Vector2f top_left, sf::Vector2f bottom_right, sf::RenderWindow& window, const float thickness)
{
	draw_thick_line(window, top_left, { bottom_right.x, top_left.y }, thickness);
	draw_thick_line(window, bottom_right, { top_left.x, bottom_right.y }, thickness);
	draw_thick_line(window, bottom_right, { bottom_right.x, top_left.y }, thickness);
	draw_thick_line(window, top_left, { top_left.x, bottom_right.y }, thickness);
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




