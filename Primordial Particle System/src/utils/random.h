#pragma once

#include <random>
#include <SFML/Graphics.hpp>

inline static std::random_device dev;
inline static std::mt19937 rng{ dev() }; // random number generator
struct Random
{
	// random engines
	inline static std::uniform_real_distribution<float> float01_dist{ 0.f, 1.f };
	inline static std::uniform_real_distribution<float> float11_dist{ -1.f, 1.f };
	inline static std::uniform_int<> int01_dist{ 0, 1 };
	inline static std::uniform_int<> int11_dist{ -1, 1 };

	// basic random functions 11 = range(-1, 1), 01 = range(0, 1)
	static float rand11_float() { return float11_dist(rng); }
	static float rand01_float() { return float01_dist(rng); }
	static int   rand01_int() { return int01_dist(rng); }
	static int   rand11_int() { return int11_dist(rng); }


	// more complex random generation. specified ranges

	template <typename Type>
	static Type rand_range(const Type min, const Type max)
	{
		// Check if the Type is an integer organism_type
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
	static sf::Color rand_color(const sf::Vector3<int> rgb_min = { 0, 0, 0 },
		const sf::Vector3<int> rgb_max = { 255 ,255, 255 })
	{
		return {
			static_cast<sf::Uint8>(rand_range(rgb_min.x, rgb_max.x)), // red value
			static_cast<sf::Uint8>(rand_range(rgb_min.y, rgb_max.y)), // dark_green value
			static_cast<sf::Uint8>(rand_range(rgb_min.x, rgb_max.z))  // blue value
		};
	}

	template<typename Type> // random SFML::Vector<Type>
	static sf::Vector2<Type> rand_vector(const Type min, const Type max)
	{
		return { rand_range(min, max), rand_range(min, max) };
	}

	template<typename Type> // random position within a rect
	static sf::Vector2<Type> rand_pos_in_rect(const sf::Rect<Type>& rect)
	{
		return { rand_range(rect.left, rect.left + rect.width),
				 rand_range(rect.top, rect.top + rect.height) };
	}

	template<typename Type> // random position within a circle
	static sf::Vector2<Type> rand_pos_in_circle(const sf::Vector2<Type> center, const float radius)
	{
		const sf::Rect<Type> rect = { center.x - radius, center.y - radius, radius * 2, radius * 2 };
		while (true)
		{
			const sf::Vector2<Type> pos = rand_pos_in_rect(rect);

			// calculating distance squares
			const sf::Vector2<Type> delta = pos - center;
			const float dist_sq = delta.x * delta.x + delta.y * delta.y;

			if (dist_sq <= radius * radius)
				return pos;
		}
	}
};