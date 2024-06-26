#pragma once
#include <SFML/Graphics.hpp>


template<typename Type>
inline sf::Vector2<Type> toroidal_direction(const sf::Vector2<Type>& start, const sf::Vector2<Type>& end, const sf::Rect<Type>& bounds)
{
    Type half_width = bounds.width * Type(0.5);
    Type half_height = bounds.height * Type(0.5);

    Type dx = end.x - start.x;
    Type dy = end.y - start.y;

    dx -= bounds.width * std::floor((dx + half_width) / bounds.width);
    dy -= bounds.height * std::floor((dy + half_height) / bounds.height);

    return { dx, dy };
}

template<typename Type>
inline Type toroidal_distance_sq(const sf::Vector2f& position1, const sf::Vector2f& position2, const sf::Rect<Type>& bounds)
{
	const sf::Vector2f dir = toroidal_direction(position1, position2, bounds);

	return dir.x * dir.x + dir.y * dir.y;
}

template<typename Type>
inline Type toroidal_distance(const sf::Vector2<Type>& vector_1, const sf::Vector2<Type>& vector_2, const sf::Rect<Type>& bounds)
{
	return sqrt(toroidal_distance_sq(vector_1, vector_2, bounds));
}

template<typename Type>
inline void border(sf::Vector2<Type>& position, const sf::Rect<Type>& bounds)
{
	position.x = std::fmod(position.x - bounds.left + bounds.width, bounds.width) + bounds.left;
	position.y = std::fmod(position.y - bounds.top + bounds.height, bounds.height) + bounds.top;
}