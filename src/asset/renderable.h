#pragma once

#include <vector>
#include <string_view>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/common.hpp"
#include "glm/geometric.hpp"
#pragma clang diagnostic pop

enum class material_type : int {
    lambertian,
    metal,
    dielectric,
    diffuse_light,
};

struct glsl_material {
    glm::vec4 albedo;
    float fuzz;
    float refraction_index;
    int32_t albedo_texture;
    material_type type;
};

glsl_material create_lambertian(glm::vec3 const& albedo);
glsl_material create_lambertian(std::string_view texture_path,
    std::vector<struct texture_data>& texture_datas);
glsl_material create_metal(glm::vec3 const& albedo, float fuzz);
glsl_material create_dielectric(float refraction_index);
glsl_material create_diffuse_light(glm::vec3 const& albedo);

struct glsl_sphere {
    glm::vec3 center;
    float radius;
    glsl_material material;
};

glsl_sphere create_sphere(
    glm::vec3 const& center, float radius, glsl_material const& material);

struct glsl_triangle_vertex {
    glm::vec4 position;
    glm::vec4 normal;
    glm::vec4 tangent;
    glm::vec2 albedo_uv;
    glm::vec2 normal_uv;
};

struct glsl_triangle {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t material;
};

struct triangle_mesh {
    std::vector<glsl_triangle_vertex> vertices;
    std::vector<glsl_triangle> triangles;
    std::vector<glsl_material> materials;
};

void add_quad(triangle_mesh& mesh, glm::vec3 const& upper_left,
    glm::vec3 const& u, glm::vec3 const& v, glsl_material const& material);

struct glsl_sky_color {
    glm::vec3 sky_color_top;
    glm::vec3 sky_color_bottom;
};
