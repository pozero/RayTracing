layout(constant_id = 0) const uint SPHERE_COUNT = 1;

struct sphere_t {
    vec3 center;
    float radius;
    material_t material;
    aabb_t aabb;
};

