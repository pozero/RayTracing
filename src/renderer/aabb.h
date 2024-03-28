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
    float x_max = std::numeric_limits<float>::lowest();
    float y_min = std::numeric_limits<float>::max();
    float y_max = std::numeric_limits<float>::lowest();
    float z_min = std::numeric_limits<float>::max();
    float z_max = std::numeric_limits<float>::lowest();
};

aabb create_aabb(std::span<vertex const> vertices);

aabb create_aabb(
    std::span<vertex const> vertices, glm::mat4 const& transformation);

aabb combine_aabb(aabb const& left, aabb const& right);

aabb combine_aabb(aabb const& box, glm::vec3 const& point);

glm::vec3 get_aabb_centroid(aabb const& aabb);

glm::vec3 get_aabb_extent(aabb const& aabb);

glm::vec3 get_aabb_max(aabb const& aabb);

glm::vec3 get_aabb_min(aabb const& aabb);

float get_aabb_surface_area(aabb const& aabb);

uint32_t get_aabb_largest_extent(aabb const& aabb);

float get_aabb_member(aabb const& aabb, uint32_t dim, uint32_t side);
