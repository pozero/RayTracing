struct ray_t {
    vec3 origin;
    vec3 direction;
};

struct intersection_t {
    vec3 point;
    vec3 normal;
    vec2 uv;
    float t;
    uint instance;
    bool front_face;
};

vec3 ray_at(const in ray_t ray,
            const in float t) {
    return ray.origin + t * ray.direction;
}

bool hit_aabb(const in aabb_t aabb,
              const in ray_t ray,
              const in vec3 inv_ray_d,
              const in ivec3 ray_dir_neg,
              in float t_min,
              in float t_max) {
    for (int a = 0; a < 3; ++ a) {
        const float inv_d = inv_ray_d[a];
        const int negative_inv_d = ray_dir_neg[a];
        const float orig = ray.origin[a];
        const vec2 interval = get_aabb_interval(aabb, a);
        const float t0 = (interval[negative_inv_d] - orig) * inv_d;
        const float t1 = (interval[1 - negative_inv_d] - orig) * inv_d;
        t_min = max(t0, t_min);
        t_max = min(t1, t_max);
        if (t_min > t_max) {
            return false;
        }
    }
    return true;
}

void intersection_set_front_face(inout intersection_t intersection, 
                                 const in ray_t ray,
                                 const in vec3 outward_normal) {
    intersection.front_face = dot(ray.direction, outward_normal) < 0.0;
    intersection.normal = intersection.front_face ? outward_normal : -outward_normal;
}

bool near_zero(const in vec3 vector) {
    const float mu = 1e-8;
    return abs(vector.x) < mu &&
           abs(vector.y) < mu &&
           abs(vector.z) < mu;
}

int max_component_index(const in vec3 vector) {
    const vec3 abs_vector = abs(vector);
    return abs_vector.x > abs_vector.y ?
               (abs_vector.x > abs_vector.z ? 0 : 2) :
               (abs_vector.y > abs_vector.z ? 1 : 2);
}

bool hit_triangle(const in triangle_t triangle,
                  const in ray_t ray,
                  const in float t_min,
                  const in float t_max,
                  out intersection_t intersection) {
    const vec3 a = triangle.a.position;
    const vec3 b = triangle.b.position;
    const vec3 c = triangle.c.position;
    // check if the triangle is degenerate
    const vec3 edge1 = b - a;
    const vec3 edge2 = c - a;
    if (abs(length(cross(edge1, edge2))) < 1e-8) {
        return false;
    }
    // transform traingle vertices to ray space
    vec3 at = a - ray.origin;
    vec3 bt = b - ray.origin;
    vec3 ct = c - ray.origin;
    // permute direction and vertices
    const int kz = max_component_index(ray.direction);
    const int kx = kz + 1 == 3 ? 0 : kz + 1;
    const int ky = kx + 1 == 3 ? 0 : kx + 1;
    const vec3 d = vec3(ray.direction[kx], ray.direction[ky], ray.direction[kz]);
    at = vec3(at[kx], at[ky], at[kz]);
    bt = vec3(bt[kx], bt[ky], bt[kz]);
    ct = vec3(ct[kx], ct[ky], ct[kz]);
    // shear to +z
    const float sx = -d.x / d.z;
    const float sy = -d.y / d.z;
    const float sz =  1.0 / d.z;
    at.x += sx * at.z;
    at.y += sy * at.z;
    bt.x += sx * bt.z;
    bt.y += sy * bt.z;
    ct.x += sx * ct.z;
    ct.y += sy * ct.z;
    // compute scaled barycentric coord
    const float e0 = bt.x * ct.y - bt.y * ct.x;
    const float e1 = ct.x * at.y - ct.y * at.x;
    const float e2 = at.x * bt.y - at.y * bt.x;
    // perform determinant tests
    if ((e0 < 0 || e1 < 0 || e2 < 0) && (e0 > 0 || e1 > 0 || e2 > 0)) {
        return false;
    }
    const float det = e0 + e1 + e2;
    if (det == 0) {
        return false;
    }
    at.z *= sz;
    bt.z *= sz;
    ct.z *= sz;
    const float t_scaled = e0 * at.z + e1 * bt.z + e2 * ct.z;
    // test against t ranger
    if (det < 0 && (t_scaled >= t_min * det || t_scaled < t_max * det)) {
        return false;
    } else if (det > 0 && (t_scaled <= t_min * det || t_scaled > t_max * det)) {
        return false;
    }
    // baricentric coordinate
    const float inv_det = 1.0 / det;
    const float b0 = e0 * inv_det;
    const float b1 = e1 * inv_det;
    const float b2 = e2 * inv_det;
    const float t = t_scaled * inv_det;
    // intersection
    const vec3 a_normal = triangle.a.normal;
    const vec3 b_normal = triangle.b.normal;
    const vec3 c_normal = triangle.c.normal;
    const vec2 a_uv = triangle.a.tex_coord;
    const vec2 b_uv = triangle.b.tex_coord;
    const vec2 c_uv = triangle.c.tex_coord;
    intersection.point = ray_at(ray, t);
    const vec3 outward_normal = b0 * a_normal + b1 * b_normal + b2 * c_normal;
    intersection_set_front_face(intersection, ray, outward_normal);
    intersection.uv = b0 * a_uv + b1 * b_uv + b2 * c_uv;
    intersection.t = t;
    return true;
}
