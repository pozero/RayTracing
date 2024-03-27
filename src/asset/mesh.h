#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#pragma clang diagnostic pop

#include <vector>
#include <string_view>

struct vertex {
    glm::vec4 position_texu;
    glm::vec4 normal_texv;
};

struct mesh {
    std::vector<vertex> vertices;
};

struct primitive {
    int32_t mesh = -1;
    int32_t transform = -1;
    int32_t material = -1;
    int32_t medium = -1;
};

mesh load_mesh(std::string_view file_path);
