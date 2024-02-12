#pragma once

#include <cstdint>
#include <string_view>
#include <memory>

enum class texture_channel {
    rgb,
    rgba,
};

struct texture_data {
    uint32_t width;
    uint32_t height;
    std::unique_ptr<uint8_t const> data;
    texture_channel channel;
};

texture_data get_texture_data(std::string_view texture_path);
