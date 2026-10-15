#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <limits>
#include <thread>

#ifdef CUSTOMCLOCK_WITH_SFML
#include <SFML/System/Time.hpp>
#endif

/**
 * @brief A single, self-consistent frame clock: delta-time stopwatch +
 *        rolling frame-rate statistics + optional frame-rate limiting.
 *
 * This replaces three previously-separate, independently-clocked classes
 * (FrameRateSmoothing, frame_rater, StopWatch). Those classes each kept
 * their own timestamp, so mixing them (e.g. calling get_delta_time() from
 * one class while pacing frames with another) could return stale or
 * inconsistent values. FrameRateSmoothing additionally used integer
 * std::chrono::milliseconds by default, so at high frame rates the elapsed
 * time between frames could truncate to 0 and its internal clock would
 * silently stop advancing.
 *
 * CustomClock fixes both problems by:
 *   1. Using a single timestamp (last_tick_time_) that every feature reads
 *      from, so delta-time, frame-rate stats, and limiting can never
 *      disagree with each other.
 *   2. Storing all durations as std::chrono::duration<double> (seconds),
 *      so there is no integer-truncation edge case at high frame rates.
 *
 * Typical usage:
 * @code
 *   CustomClock<120> clock(60.0); // 120-sample rolling window, cap at 60 fps
 *
 *   while (running)
 *   {
 *       clock.tick();                    // must be called first, once per frame
 *       float dt = clock.get_delta_time();
 *
 *       update(dt);
 *       render();
 *
 *       clock.limit_frame_rate();        // paces the loop to the target fps
 *   }
 * @endcode
 *
 * @tparam Resolution Number of samples kept for the rolling frame-rate
 *                     average (must be > 0).
 * @tparam Clock       The clock type used for timing. std::chrono::steady_clock
 *                     (the default) is monotonic and is the recommended
 *                     choice for measuring intervals; std::chrono::high_resolution_clock
 *                     is often just an alias for steady_clock or system_clock
 *                     depending on the standard library, and system_clock is
 *                     NOT monotonic (it can jump if the system time changes),
 *                     so it is deliberately not the default here.
 */
template<std::size_t Resolution = 100, typename Clock = std::chrono::steady_clock>
class CustomClock
{

public:
	/// @param target_fps Desired frame-rate cap for limit_frame_rate().
	///                    Pass 0 (or a negative number) to disable limiting.
	explicit CustomClock(double target_fps = 60.0)
		: target_frame_duration_{ target_fps > 0.0 ? 1.0 / target_fps : 0.0 }
	{
		reset();
	}

	// Non-copyable (mirrors the original classes): a clock's timestamps are
	// instance-specific state that shouldn't be silently duplicated.
	CustomClock(const CustomClock&) = delete;
	CustomClock& operator=(const CustomClock&) = delete;
	CustomClock(CustomClock&&) = default;
	CustomClock& operator=(CustomClock&&) = default;

	/// Call exactly once per frame, at the very top of the loop, before any
	/// other CustomClock method. Computes this frame's delta time and folds
	/// it into the rolling frame-rate statistics. This is the ONLY place
	/// timing state is advanced, so every other getter is guaranteed to be
	/// consistent with it.
	void tick()
	{
		const time_point now = Clock::now();
		current_delta_ = std::chrono::duration<double>(now - last_tick_time_).count();
		last_tick_time_ = now;

		// Guard against a zero (or, in theory, negative) delta -- e.g. the
		// very first tick(), or two ticks close enough together that no
		// measurable time passed. Skipping the stats update here (rather
		// than dividing by zero) is intentional and cheap; unlike the
		// original code, it does NOT stop the clock from advancing, since
		// last_tick_time_ was already updated above.
		if (current_delta_ <= 0.0)
		{
			return;
		}

		const double frame_rate = 1.0 / current_delta_;

		// in tick():
		if (current_size_ < Resolution) ++current_size_;
		else total_time_ -= frame_time_history_[current_index_]; // store dt, not 1/dt

		frame_time_history_[current_index_] = current_delta_;
		total_time_ += current_delta_;
		current_index_ = (current_index_ + 1) % Resolution;

		min_frame_rate_ = std::min(min_frame_rate_, frame_rate);
		max_frame_rate_ = std::max(max_frame_rate_, frame_rate);
	}

	// getter:
	double get_average_frame_rate() const
	{
		return (current_size_ == 0 || total_time_ <= 0.0)
			? 0.0
			: static_cast<double>(current_size_) / total_time_;
	}

	void limit_frame_rate()
	{
		if (target_frame_duration_.count() <= 0.0) return;

		const auto frame_duration = std::chrono::duration_cast<typename Clock::duration>(target_frame_duration_);
		next_frame_time_ += frame_duration;

		const time_point now = Clock::now();

		// Only forgive real stalls (we've missed at least a full frame's worth
		// of schedule) -- ordinary scheduler wake-up jitter of a fraction of a
		// frame is normal and should NOT reset the pacing, or the limiter stops
		// limiting at all on any target rate where the frame budget is small.
		if (next_frame_time_ < now - frame_duration)
		{
			next_frame_time_ = now;
		}

		std::this_thread::sleep_until(next_frame_time_);
	}

	/// Changes the target frame rate on the fly. Pass 0 (or a negative
	/// number) to disable limiting entirely.
	void set_target_fps(double fps)
	{
		target_frame_duration_ = std::chrono::duration<double>{ fps > 0.0 ? 1.0 / fps : 0.0 };
	}

	/// Delta time (seconds) of the most recently completed tick(). This is a
	/// fixed snapshot from that tick() call -- calling it multiple times in
	/// the same frame always returns the same value, unlike the original
	/// FrameRateSmoothing::get_delta_time(), which re-sampled the clock on
	/// every call.
	[[nodiscard]] float get_delta_time() const
	{
		return static_cast<float>(current_delta_);
	}

	/// Same as get_delta_time(), but full double precision -- preferable for
	/// physics or other numerically-sensitive integration.
	[[nodiscard]] double get_delta_time_precise() const
	{
		return current_delta_;
	}

	/// Time elapsed (seconds), sampled right now, since the last tick(). This
	/// number keeps growing between tick() calls, so it's useful for e.g.
	/// detecting a hung/blocked frame, but it is NOT a per-frame delta and
	/// should not be used for integration -- use get_delta_time() for that.
	[[nodiscard]] double get_time_since_last_tick() const
	{
		return std::chrono::duration<double>(Clock::now() - last_tick_time_).count();
	}

	[[nodiscard]] double get_instantaneous_frame_rate() const
	{
		if (current_size_ == 0) return 0.0;
		const double last_dt = frame_time_history_[(current_index_ + Resolution - 1) % Resolution];
		return last_dt > 0.0 ? 1.0 / last_dt : 0.0;
	}

	[[nodiscard]] double get_min_frame_rate() const
	{
		return current_size_ == 0 ? 0.0 : min_frame_rate_;
	}

	[[nodiscard]] double get_max_frame_rate() const
	{
		return max_frame_rate_;
	}

#ifdef CUSTOMCLOCK_WITH_SFML
	/// Delta time as an sf::Time, for drop-in use with SFML-based code that
	/// previously used StopWatch::get_delta_sfml(). Only available when
	/// CUSTOMCLOCK_WITH_SFML is defined before including this header, so
	/// projects that don't use SFML don't pick up the dependency.
	[[nodiscard]] sf::Time get_delta_sfml() const
	{
		return sf::seconds(static_cast<float>(current_delta_));
	}
#endif

	/// Resets all statistics and re-synchronizes both the delta-time and
	/// frame-limiting schedules to "now". Call this after a load screen, a
	/// debugger pause, or anything else that would otherwise register as one
	/// enormous delta/spike.
	void reset()
	{
		frame_time_history_.fill(0.0);
		current_index_ = 0;
		current_size_ = 0;
		total_time_ = 0.0;
		current_delta_ = 0.0;
		min_frame_rate_ = std::numeric_limits<double>::max();
		max_frame_rate_ = 0.0;

		last_tick_time_ = Clock::now();
		next_frame_time_ = last_tick_time_;
	}

private:
	using time_point = typename Clock::time_point;

	std::array<double, Resolution> frame_time_history_{};
	std::size_t current_index_ = 0;
	std::size_t current_size_ = 0;
	double total_time_ = 0.0;
	double current_delta_ = 0.0;
	double min_frame_rate_ = std::numeric_limits<double>::max();
	double max_frame_rate_ = 0.0;

	std::chrono::duration<double> target_frame_duration_;
	time_point last_tick_time_{};
	time_point next_frame_time_{};
};