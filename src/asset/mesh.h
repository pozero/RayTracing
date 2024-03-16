#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#pragma clang diagnostic pop

#include <vector>
#include <string>

struct vertex {
    glm::vec4 position_texu;
    glm::vec4 normal_texv;
};

struct mesh {
    std::vector<vertex> vertices;
};

struct instance {
    glm::mat4 transformation{1.0f};
    uint32_t mesh = std::numeric_limits<uint32_t>::max();
    uint32_t material = std::numeric_limits<uint32_t>::max();
};

mesh load_mesh(std::string_view file_path);
