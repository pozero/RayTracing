#include "high_resolution_clock.h"

high_resolution_clock::high_resolution_clock()
    : m_t0(std::chrono::high_resolution_clock::now()),
      m_delta_time(0),
      m_total_time(0) {
}

void high_resolution_clock::tick() {
    auto const t1 = std::chrono::high_resolution_clock::now();
    m_delta_time = t1 - m_t0;
    m_total_time += m_delta_time;
    m_t0 = t1;
}

void high_resolution_clock::reset() {
    m_t0 = std::chrono::high_resolution_clock::now();
    m_delta_time = {};
    m_total_time = {};
}

float high_resolution_clock::get_delta_nanoseconds() const {
    return 1.0f * (float) m_delta_time.count();
}

float high_resolution_clock::get_delta_microseconds() const {
    return 1e-3f * (float) m_delta_time.count();
}

float high_resolution_clock::get_delta_milliseconds() const {
    return 1e-6f * (float) m_delta_time.count();
}

float high_resolution_clock::get_delta_seconds() const {
    return 1e-9f * (float) m_delta_time.count();
}

float high_resolution_clock::get_total_nanoseconds() const {
    return 1.0f * (float) m_total_time.count();
}

float high_resolution_clock::get_total_microseconds() const {
    return 1e-3f * (float) m_total_time.count();
}

float high_resolution_clock::get_total_milliseconds() const {
    return 1e-6f * (float) m_total_time.count();
}

float high_resolution_clock::get_total_seconds() const {
    return 1e-9f * (float) m_total_time.count();
}
