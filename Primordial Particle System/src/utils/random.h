#pragma once

#include <random>
#include <SFML/Graphics.hpp>

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
    inline sf::Color rand_color(const sf::Vector3<int> rgb_min = { 0, 0, 0 },
        const sf::Vector3<int> rgb_max = { 255, 255, 255 })
    {
        return {
            static_cast<sf::Uint8>(rand_range(rgb_min.x, rgb_max.x)), // red value
            static_cast<sf::Uint8>(rand_range(rgb_min.y, rgb_max.y)), // green value
            static_cast<sf::Uint8>(rand_range(rgb_min.z, rgb_max.z))  // blue value
        };
    }

    template<typename Type>
    inline sf::Vector2<Type> rand_vector(const Type min, const Type max)
    {
        return { rand_range(min, max), rand_range(min, max) };
    }

    template<typename Type>
    inline sf::Vector2<Type> rand_pos_in_rect(const sf::Rect<Type>& rect)
    {
        return { rand_range(rect.left, rect.left + rect.width),
                rand_range(rect.top, rect.top + rect.height) };
    }

    template<typename Type>
    sf::Vector2<Type> rand_pos_in_circle(const sf::Vector2<Type> center, const float radius)
    {
        const sf::Rect<Type> rect = { center.x - radius, center.y - radius, radius * 2, radius * 2 };
        while (true)
        {
            const sf::Vector2<Type> pos = rand_pos_in_rect(rect);
            const sf::Vector2<Type> delta = pos - center;
            const float dist_sq = delta.x * delta.x + delta.y * delta.y;

            if (dist_sq <= radius * radius)
                return pos;
        }
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