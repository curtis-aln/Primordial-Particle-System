#pragma once

#include <random>
#include <SFML/Graphics.hpp>

// a wrapper to make generating random floats and integers more convinient
inline static std::random_device dev;
inline static std::mt19937 rng{ dev() }; // random number generator
struct RandomDist
{
	// random engines
	inline static std::uniform_real_distribution<float> float01_dist{ 0.f, 1.f };
	inline static std::uniform_real_distribution<float> float11_dist{ -1.f, 1.f };
	inline static std::uniform_int<int> int01_dist{ 0, 1 };
	inline static std::uniform_int<int> int11_dist{ -1, 1 };

	// basic random functions 11 = range(-1, 1), 01 = range(0, 1)
	static float rand11float() { return float11_dist(rng); }
	static float rand01float() { return float01_dist(rng); }
	static int   rand01int() { return int01_dist(rng); }
	static int   rand11int() { return int11_dist(rng); }


	// more complex random generation. specified ranges

	template <typename Type>
	static Type randRange(const Type min, const Type max)
	{
		// Check if the Type is an integer type
		if constexpr (std::is_integral_v<Type>)
		{
			std::uniform_int_distribution<Type> int_dist{ min, max };
			return int_dist(rng);
		}
		else
		{
			std::uniform_real_distribution<Type> float_dist{ min, max };
			return float_dist(rng);
		}
	}

	// random SFML::Vector<Type>
	static sf::Color randColor(const sf::Vector3<int> rgb_min = { 0, 0, 0 },
		const sf::Vector3<int> rgb_max = { 255 ,255, 255 })
	{
		return {
			static_cast<sf::Uint8>(randRange(rgb_min.x, rgb_max.x)), // red value
			static_cast<sf::Uint8>(randRange(rgb_min.y, rgb_max.y)), // green value
			static_cast<sf::Uint8>(randRange(rgb_min.x, rgb_max.z))  // blue value
		};
	}

	template<typename Type> // random SFML::Vector<Type>
	static sf::Vector2<Type> randVector(const Type min, const Type max)
	{
		return { randRange(min, max), randRange(min, max) };
	}

	template<typename Type> // random position inside of a rect
	static sf::Vector2<Type> randPosInRect(const sf::Rect<Type>& rect)
	{
		return { randRange(rect.left, rect.left + rect.width),
				 randRange(rect.top, rect.top + rect.height) };
	}
};