#pragma once

#include <cstdint>

enum sky_light_type : int32_t {
    uniform = 0,
    textured
};

struct sky_light {
    float intensity;
    sky_light_type type;
};

enum light_type : int32_t {
    distant = 0,
    area
};

struct light {
    float intensity;
    light_type type;
};
