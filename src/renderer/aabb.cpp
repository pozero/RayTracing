#include "aabb.h"
#include "check.h"

aabb create_aabb(std::span<vertex const> vertices) {
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

aabb create_aabb(
    std::span<vertex const> vertices, glm::mat4 const& transformation) {
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

aabb combine_aabb(aabb const& box, glm::vec3 const& point) {
    aabb combined{};
    combined.x_min = std::min(box.x_min, point.x);
    combined.x_max = std::max(box.x_max, point.x);
    combined.y_min = std::min(box.y_min, point.y);
    combined.y_max = std::max(box.y_max, point.y);
    combined.z_min = std::min(box.z_min, point.z);
    combined.z_max = std::max(box.z_max, point.z);
    return combined;
}

glm::vec3 get_aabb_centroid(aabb const& aabb) {
    return glm::vec3{
        0.5f * (aabb.x_min + aabb.x_max),
        0.5f * (aabb.y_min + aabb.y_max),
        0.5f * (aabb.z_min + aabb.z_max),
    };
}

glm::vec3 get_aabb_extent(aabb const& aabb) {
    return glm::vec3{
        aabb.x_max - aabb.x_min,
        aabb.y_max - aabb.y_min,
        aabb.z_max - aabb.z_min,
    };
}

glm::vec3 get_aabb_max(aabb const& aabb) {
    return glm::vec3{
        aabb.x_max,
        aabb.y_max,
        aabb.z_max,
    };
}

glm::vec3 get_aabb_min(aabb const& aabb) {
    return glm::vec3{
        aabb.x_min,
        aabb.y_min,
        aabb.z_min,
    };
}

float get_aabb_surface_area(aabb const& aabb) {
    float const x_interval = aabb.x_max - aabb.x_min;
    float const y_interval = aabb.y_max - aabb.y_min;
    float const z_interval = aabb.z_max - aabb.z_min;
    return 2 * (x_interval * y_interval + y_interval * z_interval +
                   z_interval * x_interval);
}

uint32_t get_aabb_largest_extent(aabb const& aabb) {
    float const x_interval = aabb.x_max - aabb.x_min;
    float const y_interval = aabb.y_max - aabb.y_min;
    float const z_interval = aabb.z_max - aabb.z_min;
    if (x_interval > y_interval && x_interval > z_interval) {
        return 0;
    } else if (y_interval > z_interval) {
        return 1;
    } else {
        return 2;
    }
}

#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
float get_aabb_member(aabb const& aabb, uint32_t dim, uint32_t side) {
    CHECK(0 <= dim && dim <= 2, "");
    CHECK(side == 0 || side == 1, "");
    if (dim == 0) {
        if (side == 0) {
            return aabb.x_min;
        } else {
            return aabb.x_max;
        }
    } else if (dim == 1) {
        if (side == 0) {
            return aabb.y_min;
        } else {
            return aabb.y_max;
        }
    } else {
        if (side == 0) {
            return aabb.z_min;
        } else {
            return aabb.z_max;
        }
    }
}
