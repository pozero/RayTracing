#pragma once

#include <cstdint>
#include <string_view>
#include <memory>

enum class texture_channel {
    rgb,
    rgba,
};

enum class texture_format {
    unorm,
    sfloat,
};

struct texture_data {
    uint32_t width;
    uint32_t height;
    void* data;
    texture_channel channel;
    texture_format format;
};

texture_data get_texture_data(std::string_view texture_path);
