#pragma once

#include <vector>
#include <string_view>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/common.hpp"
#pragma clang diagnostic pop

enum class material_type {
    lambertian,
    metal,
    dielectric,
};

struct glsl_material {
    material_type type;
    alignas(sizeof(glm::vec4)) glm::vec3 albedo;
    float fuzz;
    float refraction_index;
    int albedo_texture;
};

glsl_material create_lambertian(glm::vec3 const& albedo);
glsl_material create_lambertian(std::string_view texture_path,
    std::vector<struct texture_data>& texture_datas);
glsl_material create_metal(glm::vec3 const& albedo, float fuzz);
glsl_material create_dielectric(float refraction_index);

struct glsl_sphere {
    glm::vec3 center;
    float radius;
    glsl_material material;
};

glsl_sphere create_sphere(
    glm::vec3 const& center, float radius, glsl_material const& material);
