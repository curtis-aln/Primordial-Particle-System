#pragma once
#include <SFML/Graphics.hpp>

template<typename Type>
sf::Vector2<Type> toroidal_direction(const sf::Vector2<Type>& start, const sf::Vector2<Type>& end, const sf::Rect<Type>& bounds)
{
	Type dist_x = abs(start.x - end.x);
	Type dist_y = abs(start.y - end.y);

	if (dist_x < bounds.width / 2)
		dist_x = dist_x;
	else
		dist_x = dist_x - bounds.width;

	if (start.x > end.x)
		dist_x *= -1;


	if (dist_y < bounds.height / 2)
		dist_y = dist_y;
	else
		dist_y = dist_y - bounds.height;

	if (start.y > end.y)
		dist_y *= -1;

	return  { dist_x, dist_y };
}

template<typename Type>
Type toroidal_distance_sq(const sf::Vector2f& position1, const sf::Vector2f& position2, const sf::Rect<Type>& bounds)
{
	const sf::Vector2f dir = toroidal_direction(position1, position2, bounds);

	return dir.x * dir.x + dir.y * dir.y;
}

template<typename Type>
Type toroidal_distance(const sf::Vector2<Type>& vector_1, const sf::Vector2<Type>& vector_2, const sf::Rect<Type>& bounds)
{
	return sqrt(toroidal_distance_sq(vector_1, vector_2, bounds));
}

template<typename Type>
void border(sf::Vector2<Type>& position, const sf::Rect<Type>& bounds)
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