#pragma once
#include <SFML/Graphics.hpp>

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

inline double wrappedDistSq(const sf::Vector2f pos1, const sf::Vector2f pos2, const unsigned screenWidth, const unsigned screenHeight) {
	float dx = abs(pos2.x - pos1.x);
	float  dy = abs(pos2.y - pos1.y);

	if (dx > screenWidth / 2)
		dx = screenWidth - dx;
	if (dy > screenHeight / 2)
		dy = screenHeight - dy;

	return dx * dx + dy * dy;
}

inline sf::Vector2f directionCalculator(const sf::Vector2f pos1, const sf::Vector2f pos2, const unsigned screenWidth, const unsigned screenHeight) {
	const float StartX = pos1.x, StartY = pos1.y;
	const float EndX = pos2.x,  EndY = pos2.y;

	const sf::Vector2f sim_bounds = {
		static_cast<float>(screenWidth),
		static_cast<float>(screenHeight) };

	const float dist_x = abs(StartX - EndX);
	float result_x;

	if (dist_x < sim_bounds.x / 2)
		result_x = dist_x;
	else
		result_x = dist_x - sim_bounds.x;

	if (StartX > EndX)
		result_x *= -1;


	const float dist_y = abs(StartY - EndY);
	float result_y;

	if (dist_y < sim_bounds.y / 2)
		result_y = dist_y;
	else
		result_y = dist_y - sim_bounds.y;

	if (StartY > EndY)
		result_y *= -1;

	return {result_x, result_y};
}

inline bool isOnRightHemisphere(const sf::Vector2f& pos, float angle, const sf::Vector2f& other_pos, const unsigned screenWidth, const unsigned screenHeight)
{
	// Calculate the x and y coordinates of other_pos relative to pos
	const sf::Vector2f relative = directionCalculator(pos, other_pos, screenWidth, screenHeight);

	// Calculate the angle between the positive x-axis and the vector to other_pos
	float other_angle = std::atan2(relative.y, relative.x);

	// Ensure the angle is between 0 and 2*pi
	if (other_angle < 0) other_angle += 2 * pi;
	if (other_angle > 2 * pi) other_angle -= 2 * pi;

	// Check if other_pos lies on the right hemisphere
	if (other_angle >= angle && other_angle <= (angle + pi)) {
		return true;
	}

	return false;
}



inline void border(sf::Vector2f& position, const sf::Rect<float>& bounds)
{
	if (position.x < bounds.left)
	{
		position.x = bounds.width;
	}

	else if (position.x > bounds.left + bounds.width)
	{
		position.x = bounds.left;
	}

	if (position.y < bounds.top)
	{
		position.y = bounds.height;
	}

	else if (position.y > bounds.top + bounds.height)
	{
		position.y = bounds.top;
	}
}

inline float distSq(const sf::Vector2f vec1, const sf::Vector2f vec2)
{
	const sf::Vector2f delta = vec2 - vec1;
	return delta.x * delta.x + delta.y * delta.y;
}

inline sf::VertexArray createLine(const sf::Vector2f& position, const float angleRadians, const float length) {
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

inline sf::VertexArray createLine(const sf::Vector2f& pos1, const sf::Vector2f& pos2) {
	sf::VertexArray line(sf::Lines, 2);

	// Set the vertices of the line segment
	line[0].position = pos1;
	line[1].position = pos2;
	line[0].color = { 0, 255, 0 };
	line[1].color = { 0, 255, 0 };

	return line;
}




