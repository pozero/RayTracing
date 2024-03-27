#pragma once

#include <limits>
#include <span>

#include "asset/mesh.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#pragma clang diagnostic pop

struct aabb {
    float x_min = std::numeric_limits<float>::max();
    float x_max = std::numeric_limits<float>::min();
    float y_min = std::numeric_limits<float>::max();
    float y_max = std::numeric_limits<float>::min();
    float z_min = std::numeric_limits<float>::max();
    float z_max = std::numeric_limits<float>::min();
};

aabb create_aabb(std::span<vertex> vertices);

aabb create_aabb(std::span<vertex> vertices, glm::mat4 const& transformation);

aabb combine_aabb(aabb const& left, aabb const& right);

glm::vec3 get_aabb_centroid(aabb const& aabb);
