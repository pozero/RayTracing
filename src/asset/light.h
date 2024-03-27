#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec3.hpp"
#pragma clang diagnostic pop

#include <cstdint>

struct sky_light {
    glm::vec3 intensity{0.0f, 0.0f, 0.0f};
    int32_t environment_tex = -1;
};

enum light_type : int32_t {
    distant = 0,
    area
};

struct light {
    glm::vec3 intensity;
    float _p0 = 0.0f;

    glm::vec3 direction;
    float _p1 = 0.0f;

    int32_t mesh = -1;
    int32_t transform = -1;
    int32_t emission_tex = -1;
    light_type type;
};
