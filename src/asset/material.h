#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec3.hpp"
#pragma clang diagnostic pop

struct material {
    glm::vec3 albedo{0.82f, 0.67f, 0.16f};
    float albedo_tex = 0.0f;

    float metallic = 0.0f;
    float subsurface = 0.0f;
    float specular = 0.5f;
    float roughness = 0.5f;

    float specular_tint = 0.0f;
    float anisotropic = 0.0f;
    float sheen = 0.0f;
    float sheen_tint = 0.5f;

    float clearcoat = 0.0f;
    float clearcoat_gloss = 1.0f;
    float _p0[2]{};
};
