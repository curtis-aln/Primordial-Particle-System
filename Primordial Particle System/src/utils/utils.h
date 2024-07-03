#pragma once
#include <SFML/Graphics.hpp>
#include "toroidal_space.h"

inline static constexpr float pi = 3.141592653589793238462643383279502884197f;
inline static constexpr float TWO_PI = 2.f * pi;

inline int sign(const int x)
{
	return (x > 0) ? 1 : (x < 0) ? -1 : 0;
}


inline bool isOnRightHemisphere(const sf::Vector2f& direction, float angle)
{

	// Calculate the angle between the positive x-axis and the vector to other_pos
	float other_angle = std::atan2(direction.y, direction.x);

	// Normalize angles to the range [0, TWO_PI)
	other_angle = (other_angle < 0) ? other_angle + TWO_PI : other_angle;
	angle = std::fmod(angle, TWO_PI);
	angle = (angle < 0) ? angle + TWO_PI : angle;

	// Calculate the right hemisphere range
	const float right_start = angle;
	const float right_end = std::fmod(angle + pi, TWO_PI);

	// Check if other_angle lies on the right hemisphere
	return (right_start < right_end) ? (other_angle >= right_start && other_angle <= right_end) : (other_angle >= right_start || other_angle <= right_end);
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




