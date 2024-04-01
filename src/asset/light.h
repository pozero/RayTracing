#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec3.hpp"
#pragma clang diagnostic pop

#include <cstdint>

enum light_type : int32_t {
    distant = 0,
    area_single_sided,
    area_double_sided,
    sky
};

struct light {
    glm::vec3 intensity;
    int32_t emission_tex = -1;

    glm::vec3 direction;
    light_type type;

    uint32_t mesh = 0;
    int32_t transform = -1;
    float _p0[2]{};
};
