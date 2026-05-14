#pragma once

#include <array>
#include <chrono>
#include <limits>
#include <stdexcept>

template<size_t Resolution, typename Duration = std::chrono::milliseconds>
class FrameRateSmoothing
{
public:
    FrameRateSmoothing()
	{
        if (Resolution == 0) 
        {
            throw std::invalid_argument("Resolution must be greater than 0");
        }
        reset();
    }

    FrameRateSmoothing(const FrameRateSmoothing&) = delete;
    FrameRateSmoothing& operator=(const FrameRateSmoothing&) = delete;
    FrameRateSmoothing(FrameRateSmoothing&&) = default;
    FrameRateSmoothing& operator=(FrameRateSmoothing&&) = default;

    void update_frame_rate()
	{
		const auto current_time = std::chrono::steady_clock::now();
        const auto elapsed_time = std::chrono::duration_cast<Duration>(current_time - last_frame_time_);

        if (elapsed_time.count() > 0) 
        {
            double frame_rate = static_cast<double>(Duration::period::den) /
                static_cast<double>(elapsed_time.count() * Duration::period::num);

            if (current_size_ < Resolution) 
            {
                ++current_size_;
            }
            else 
            {
                total_sum_ -= frame_rates_array_[current_index_];
            }

            frame_rates_array_[current_index_] = frame_rate;
            total_sum_ += frame_rate;
            current_index_ = (current_index_ + 1) % Resolution;

            last_frame_time_ = current_time;

            min_frame_rate_ = std::min(min_frame_rate_, frame_rate);
            max_frame_rate_ = std::max(max_frame_rate_, frame_rate);
        }
    }

    [[nodiscard]] double get_average_frame_rate() const
	{
        return current_size_ == 0 ? 0.0 : total_sum_ / static_cast<double>(current_size_);
    }

    [[nodiscard]] double get_instantaneous_frame_rate() const
	{
        return current_size_ == 0 ? 0.0 : frame_rates_array_[(current_index_ + Resolution - 1) % Resolution];
    }

    float get_delta_time() const
    {
        return std::chrono::duration<float>(std::chrono::steady_clock::now() - last_frame_time_).count();
    }



    [[nodiscard]] double get_min_frame_rate() const { return min_frame_rate_; }
    [[nodiscard]] double get_max_frame_rate() const { return max_frame_rate_; }

    void reset()
	{
        frame_rates_array_.fill(0.0);
        current_index_ = 0;
        current_size_ = 0;
        total_sum_ = 0.0;
        last_frame_time_ = std::chrono::steady_clock::now();
        min_frame_rate_ = std::numeric_limits<double>::max();
        max_frame_rate_ = 0.0;
    }

private:
    std::array<double, Resolution> frame_rates_array_;
    size_t current_index_ = 0;
    size_t current_size_ = 0;
    double total_sum_ = 0.0;
    std::chrono::steady_clock::time_point last_frame_time_;
    double min_frame_rate_ = std::numeric_limits<double>::max();
    double max_frame_rate_ = 0.0;
};