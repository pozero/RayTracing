#pragma once

#include <chrono>

class high_resolution_clock {
public:
    high_resolution_clock();

    // Tick the high resolution clock.
    // Tick the clock before reading the delta time for the first time.
    // Only tick the clock once per frame.
    void tick();

    // Reset the clock.
    void reset();

    float get_delta_nanoseconds() const;
    float get_delta_microseconds() const;
    float get_delta_milliseconds() const;
    float get_delta_seconds() const;

    float get_total_nanoseconds() const;
    float get_total_microseconds() const;
    float get_total_milliseconds() const;
    float get_total_seconds() const;

private:
    // Initial time point.
    std::chrono::high_resolution_clock::time_point m_t0;
    // Time since last tick.
    std::chrono::high_resolution_clock::duration m_delta_time;
    std::chrono::high_resolution_clock::duration m_total_time;
};
