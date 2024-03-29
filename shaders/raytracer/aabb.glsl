struct aabb_t {
    float x_min;
    float x_max;
    float y_min;
    float y_max;
    float z_min;
    float z_max;
};

vec2 get_aabb_interval(const in aabb_t aabb,
                       const in int index) {
    if (index == 1) {
        return vec2(aabb.y_min, aabb.y_max);
    } else if (index == 2) {
        return vec2(aabb.z_min, aabb.z_max);
    }
    return vec2(aabb.x_min, aabb.x_max);
}

bool interval_intersect(const in vec2 a,
                        const in vec2 b) {
    const float f = max(a.x, b.x);
    const float F = max(a.y, b.y);
    return f < F;
}

bool aabb_intersect(const in aabb_t a,
                    const in aabb_t b) {
    const bool x_intersect = interval_intersect(get_aabb_interval(a, 0), get_aabb_interval(b, 0));
    const bool y_intersect = interval_intersect(get_aabb_interval(a, 1), get_aabb_interval(b, 1));
    const bool z_intersect = interval_intersect(get_aabb_interval(a, 2), get_aabb_interval(b, 2));
    return x_intersect && y_intersect && z_intersect;
}

vec3 aabb_lowest_point(const in aabb_t aabb) {
    return vec3(aabb.x_min,
                aabb.y_min,
                aabb.z_min);
}

vec3 aabb_highest_point(const in aabb_t aabb) {
    return vec3(aabb.x_max,
                aabb.y_max,
                aabb.z_max);
}
