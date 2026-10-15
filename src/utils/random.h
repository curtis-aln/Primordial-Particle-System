#pragma once

#include <random>

#include <cstdint>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector3.hpp>
#include <stdexcept>
#include <string.h>
#include <type_traits>
#include <vector>

namespace Random
{
	inline thread_local std::mt19937 rng{ std::random_device{}() };

	// random engines
	inline thread_local std::uniform_real_distribution<float> float01_dist{ 0.f, 1.f };
	inline thread_local std::uniform_real_distribution<float> float11_dist{ -1.f, 1.f };
	inline thread_local std::uniform_int_distribution<int> int01_dist{ 0, 1 };
	inline thread_local std::uniform_int_distribution<int> int11_dist{ -1, 1 };

	// basic random functions 11 = range(-1, 1), 01 = range(0, 1)
	inline float rand11_float() { return float11_dist(rng); }
	inline float rand01_float() { return float01_dist(rng); }
	inline int   rand01_int() { return int01_dist(rng); }
	inline int   rand11_int() { return int11_dist(rng); }



	// more complex random generation. specified ranges
	template <typename Type>
	Type rand_range(const Type min, const Type max)
	{
		if constexpr (std::is_integral_v<Type>)
		{
			// For integers, use rejection sampling to avoid distribution construction.
			// This is unbiased as long as the range fits in uint32 which it always will for ARIA.
			const auto range = static_cast<uint32_t>(max - min) + 1u;
			const uint32_t threshold = (UINT32_MAX - range + 1u) % range;
			uint32_t r;
			do { r = rng(); } while (r < threshold);
			return min + static_cast<Type>(r % range);
		}
		else
		{
			// Map raw mt19937 output to [0,1) then scale — no distribution object.
			constexpr float scale = 1.f / static_cast<float>(0xFFFFFFFFu);
			const float t = static_cast<float>(rng()) * scale;
			return min + t * (max - min);
		}
	}

	inline uint8_t rand_byte() { return static_cast<uint8_t>(rand_range(0, 255)); }

	template <typename Type>
	Type rand_val_in_vector(const std::vector<Type>& vector)
	{
		if (vector.empty()) {
			throw std::runtime_error("rand_val_in_vector called with an empty vector.");
		}

		const size_t index = Random::rand_range(size_t(0), vector.size() - size_t(1));
		return vector.at(index);
	}

	// random SFML::Vector<Type>
	inline sf::Color rand_color(const sf::Vector3<int> rgb_min = { 0, 0, 0 },
		const sf::Vector3<int> rgb_max = { 255, 255, 255 })
	{
		return {
			static_cast<std::uint8_t>(rand_range(rgb_min.x, rgb_max.x)), // red value
			static_cast<std::uint8_t>(rand_range(rgb_min.y, rgb_max.y)), // green value
			static_cast<std::uint8_t>(rand_range(rgb_min.z, rgb_max.z))  // blue value
		};
	}

	template<typename Type>
	sf::Vector2<Type> rand_vector(const Type min, const Type max)
	{
		return { rand_range(min, max), rand_range(min, max) };
	}

	template<typename Type>
	sf::Vector2<Type> rand_pos_in_rect(const sf::Rect<Type>& rect)
	{
		return { rand_range(rect.position.x, rect.position.x + rect.size.x),
				rand_range(rect.position.y, rect.position.y + rect.size.y) };
	}

	template<typename Type>
	sf::Vector2<Type> rand_position_in_circle(const sf::Vector2<Type>& center, float radius)
	{
		float u = rand01_float(); // [0,1]
		float v = rand01_float(); // [0,1]

		float r = radius * std::sqrt(u);
		float theta = 2.0f * 3.1415926535f * v;

		return {
			center.x + r * std::cos(theta),
			center.y + r * std::sin(theta)
		};
	}

	inline void set_seed(const unsigned int seed = std::random_device{}())
	{
		rng.seed(seed);
	}

	inline std::mt19937& get_engine()
	{
		return rng;
	}


}


namespace FastRandom
{
	// Not statistically rigorous — just a cheap, fast, "good enough" bit-mixer.
	// 4 bytes of state vs mt19937's ~2.5KB: far friendlier to cache, no rejection
	// loops, no distribution machinery.
	inline thread_local uint32_t fast_state = std::random_device{}() | 1u; // must never be 0

	inline uint32_t next_bits()
	{
		uint32_t x = fast_state;
		x ^= x << 13;
		x ^= x >> 17;
		x ^= x << 5;
		return fast_state = x;
	}

	// Bit-trick float in [0,1): stuff 23 random mantissa bits into a float
	// with exponent forced to represent [1,2), then subtract 1. No division,
	// no branches, no rejection sampling.
	inline float fast01()
	{
		uint32_t bits = (next_bits() >> 9) | 0x3f800000u;
		float f;
		std::memcpy(&f, &bits, sizeof(f));
		return f - 1.f;
	}

	inline void set_seed(uint32_t seed)
	{
		fast_state = seed | 1u;
	}
}