#pragma once

#include <cstdint>

struct render_options {
    uint32_t resolution_x = 1280;
    uint32_t resolution_y = 720;
    uint32_t max_depth = 5;
    uint32_t tile_width = 256;
    uint32_t tile_height = 144;
};
