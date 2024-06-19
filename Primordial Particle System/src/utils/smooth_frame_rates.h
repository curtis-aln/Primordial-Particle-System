#pragma once

#include <array> // Include for std::array
#include <chrono> // Include for std::chrono

// A utility class for smoothing out frame rates by calculating the average frame rate across multiple frames
template<size_t Resolution>
class FrameRateSmoothing
{
    std::array<unsigned, Resolution> frame_rates_array_;
    size_t current_index_ = 0;
    size_t current_size_ = 0;
    std::chrono::steady_clock::time_point last_frame_time_;

public:
    // Constructor
    FrameRateSmoothing() : frame_rates_array_{}, last_frame_time_(std::chrono::steady_clock::now()) {}

    // Get the average frame rate
    [[nodiscard]] float get_average_frame_rate() const
    {
        if (current_size_ == 0) return 0.0f;

        unsigned sum = 0;
        for (size_t i = 0; i < current_size_; ++i)
            sum += frame_rates_array_[i];

        return static_cast<float>(sum) / static_cast<float>(current_size_);
    }

    // Update frame rate array
    void update_frame_rate()
    {
        const auto current_time = std::chrono::steady_clock::now();
        const auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_frame_time_);

        if (elapsed_time.count() > 0)
        {
            unsigned frame_rate = 1000 / static_cast<unsigned>(elapsed_time.count());

            if (current_size_ < Resolution)
                ++current_size_;

            frame_rates_array_[current_index_] = frame_rate;
            current_index_ = (current_index_ + 1) % Resolution;

            last_frame_time_ = current_time;
        }
    }
};