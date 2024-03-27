#include "aabb.h"

aabb create_aabb(std::span<vertex> vertices) {
    aabb aabb{};
    for (auto const& v : vertices) {
        float const x = v.position_texu.x;
        float const y = v.position_texu.y;
        float const z = v.position_texu.z;
        aabb.x_min = std::min(x, aabb.x_min);
        aabb.x_max = std::max(x, aabb.x_max);
        aabb.y_min = std::min(y, aabb.y_min);
        aabb.y_max = std::max(y, aabb.y_max);
        aabb.z_min = std::min(z, aabb.z_min);
        aabb.z_max = std::max(z, aabb.z_max);
    }
    return aabb;
}

aabb create_aabb(std::span<vertex> vertices, glm::mat4 const& transformation) {
    aabb aabb{};
    for (auto const& v : vertices) {
        glm::vec4 position{
            v.position_texu.x, v.position_texu.y, v.position_texu.z, 1.0f};
        position = transformation * position;
        float const x = position.x;
        float const y = position.y;
        float const z = position.z;
        aabb.x_min = std::min(x, aabb.x_min);
        aabb.x_max = std::max(x, aabb.x_max);
        aabb.y_min = std::min(y, aabb.y_min);
        aabb.y_max = std::max(y, aabb.y_max);
        aabb.z_min = std::min(z, aabb.z_min);
        aabb.z_max = std::max(z, aabb.z_max);
    }
    return aabb;
}

aabb combine_aabb(aabb const& left, aabb const& right) {
    aabb combined{};
    combined.x_min = std::min(left.x_min, right.x_min);
    combined.x_max = std::max(left.x_max, right.x_max);
    combined.y_min = std::min(left.y_min, right.y_min);
    combined.y_max = std::max(left.y_max, right.y_max);
    combined.z_min = std::min(left.z_min, right.z_min);
    combined.z_max = std::max(left.z_max, right.z_max);
    return combined;
}

glm::vec3 get_aabb_centroid(aabb const& aabb) {
    return glm::vec3{
        0.5f * (aabb.x_min + aabb.x_max),
        0.5f * (aabb.y_min + aabb.y_max),
        0.5f * (aabb.z_min + aabb.z_max),
    };
}
