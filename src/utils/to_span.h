#pragma once

#include <vector>
#include <span>
#include <cstdint>

template <typename T>
uint32_t size_in_byte(std::vector<T> const& vec) {
    return (uint32_t) vec.size() * sizeof(T);
}

template <typename T>
std::span<const uint8_t> to_span(T const& obj) {
    return std::span<const uint8_t>{
        reinterpret_cast<const uint8_t*>(&obj), sizeof(T)};
}

template <typename T>
std::span<const uint8_t> to_span(std::vector<T> const& vec) {
    return std::span<const uint8_t>{
        reinterpret_cast<const uint8_t*>(vec.data()), vec.size() * sizeof(T)};
}
