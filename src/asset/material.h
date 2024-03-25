#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec3.hpp"
#pragma clang diagnostic pop

struct material {
    glm::vec3 albedo{0.82f, 0.67f, 0.16f};
    int32_t albedo_tex = -1;

    glm::vec3 emission{0.0f, 0.0f, 0.0f};
    int32_t emission_tex = -1;

    float metallic = 0.0f;
    float subsurface = 0.0f;
    float ior = 1.5f;
    float roughness = 0.5f;

    float specular_tint = 0.0f;
    float anisotropic = 0.0f;
    float sheen = 0.0f;
    float sheen_tint = 0.5f;

    float clearcoat = 0.0f;
    float clearcoat_gloss = 1.0f;
    int32_t normal_tex = -1;
    int32_t metallic_roughness_tex = -1;
};

enum class medium_type : int32_t {
    absorption = 0,
    emission,
    scattering
};

struct medium {
    glm::vec3 color{};
    float density;
    float anisotropic;
    medium_type type;
};
