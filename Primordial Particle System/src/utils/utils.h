#pragma once
#include <SFML/Graphics.hpp>
#include "toroidal_space.h"

inline static constexpr float pi = 3.141592653589793238462643383279502884197f;


int sign(const int x)
{
	return (x > 0) ? 1 : (x < 0) ? -1 : 0;
}

void clamp_angle(float& angle_radians)
{
	while (angle_radians < 0)    angle_radians += 2*pi;
	while (angle_radians > 2*pi) angle_radians -= 2*pi;
}

inline bool isOnRightHemisphere(const sf::Vector2f& pos, float angle, const sf::Vector2f& other_pos, const sf::FloatRect& bounds)
{
	// Calculate the x and y coordinates of other_pos relative to pos
	const sf::Vector2f relative = toroidal_direction(pos, other_pos, bounds);

	// Calculate the angle between the positive x-axis and the vector to other_pos
	float other_angle = std::atan2(relative.y, relative.x);

	// Normalize angles to the range [0, 2*pi)
	if (other_angle < 0) other_angle += 2 * pi;

	// Normalize angle to [0, 2*pi)
	angle = std::fmod(angle, 2 * pi);
	if (angle < 0) angle += 2 * pi;

	// Calculate the right hemisphere range
	float right_start = angle;
	float right_end = std::fmod(angle + pi, 2 * pi);

	// Check if other_angle lies on the right hemisphere
	if (right_start < right_end) {
		return (other_angle >= right_start && other_angle <= right_end);
	}
	else {
		return (other_angle >= right_start || other_angle <= right_end);
	}
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




