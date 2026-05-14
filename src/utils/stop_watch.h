#pragma once

#include <chrono>
#include <SFML/System/Time.hpp>

class StopWatch
{
    std::chrono::high_resolution_clock::time_point start_time_;

public:
    StopWatch() : start_time_(std::chrono::high_resolution_clock::now()) {}

    double get_delta()
    {
        const auto current_time = std::chrono::high_resolution_clock::now();
        const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time_).count();
        start_time_ = current_time;
        return micros * 1e-6;
    }

    sf::Time get_delta_sfml()
    {
        return sf::seconds(static_cast<float>(get_delta()));
    }
};