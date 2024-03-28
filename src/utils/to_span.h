#pragma once

#include <vector>
#include <span>
#include <cstdint>

template <typename T>
uint32_t size_in_byte(std::vector<T> const& vec) {
    return (uint32_t) vec.size() * sizeof(T);
}

template <typename T>
std::span<uint8_t const> to_byte_span(T const& obj) {
    return std::span<uint8_t const>{
        reinterpret_cast<uint8_t const*>(&obj), sizeof(T)};
}

template <typename T>
std::span<uint8_t const> to_byte_span(std::vector<T> const& vec) {
    return std::span<uint8_t const>{
        reinterpret_cast<uint8_t const*>(vec.data()), vec.size() * sizeof(T)};
}

template <typename T>
std::span<T> to_span(std::vector<T>& vec) {
    return std::span<T>{vec.data(), vec.size()};
}

template <typename T>
std::span<T const> to_span(std::vector<T> const& vec) {
    return std::span<T const>{vec.data(), vec.size()};
}
