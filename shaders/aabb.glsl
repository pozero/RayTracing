struct aabb_t {
    vec2 x_interval;
    vec2 y_interval;
    vec2 z_interval;
};

vec2 get_aabb_interval(const in aabb_t aabb,
                       const in int index) {
    if (index == 1) {
        return aabb.y_interval;
    } else if (index == 2) {
        return aabb.z_interval;
    }
    return aabb.x_interval;
}

bool interval_intersect(const in vec2 a,
                        const in vec2 b) {
    const float f = max(a.x, b.x);
    const float F = max(a.y, b.y);
    return f < F;
}

bool aabb_intersect(const in aabb_t a,
                    const in aabb_t b) {
    const bool x_intersect = interval_intersect(a.x_interval, b.x_interval);
    const bool y_intersect = interval_intersect(a.y_interval, b.y_interval);
    const bool z_intersect = interval_intersect(a.z_interval, b.z_interval);
    return x_intersect && y_intersect && z_intersect;
}

vec3 aabb_lowest_point(const in aabb_t aabb) {
    return vec3(world_aabb.x_interval.x,
                world_aabb.y_interval.x,
                world_aabb.z_interval.x);
}

vec3 aabb_highest_point(const in aabb_t aabb) {
    return vec3(world_aabb.x_interval.y,
                world_aabb.y_interval.y,
                world_aabb.z_interval.y);
}
