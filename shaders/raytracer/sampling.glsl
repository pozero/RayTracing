float power_heuristic(const in float a,
                      const in float b) {
    const float a_squared = a * a;
    return a_squared / (a_squared + b * b);
}

vec3 uniform_sampling_triangle() {
    const float r0 = rand_01();
    const float r1 = rand_01();
    float b0 = 0;
    float b1 = 0;
    if (r0 < r1) {
        b0 = 0.5 * r0;
        b1 = r1 - b0;
    } else {
        b1 = 0.5 * r1;
        b0 = r0 - b1;
    }
    return vec3(b0, b1, (1 - b0 - b1));
}

vec3 uniform_sampling_sphere() {
    const float r0 = rand_01();
    const float r1 = rand_01();
    const float cos_theta = 1 - 2 * r0;
    const float sin_theta = sqrt(1 - square(cos_theta));
    const float phi = TWO_PI * r1;
    return vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
}

vec3 cosine_sampling_sphere() {
    const float r0 = rand_01();
    const float r1 = rand_01();
    const float r = sqrt(r0);
    const float phi = TWO_PI * r1;
    const float x = r * cos(phi);
    const float y = r * sin(phi);
    const float z = sqrt(max(0, 1 - square(x), square(y)));
    return vec3(x, y, z);
}

vec3 sample_ggx_visible_ndf(const in vec3 wo,
                            const in float ax,
                            const in float ay) {
    const float r0 = rand_01();
    const float r1 = rand_01();
    const vec3 v = normalize(vec3(ax * wo.x, wo.y, ay * wo.z));
    const vec3 t1 = (v.y < 0.9999) ?
        normalize(cross(v, vec3(0, 1, 0))) :
        normalize(cross(v, vec3(1, 0, 0)));
    const vec3 t2 = cross(t1, v);
    const float a = 1 / (1 + v.y);
    const float r = sqrt(r0);
    const float phi = (r1 < a) ?
        (r1 / a) * PI :
        ((r1 - a) / (1 - a)) * PI + PI;
    const float p1 = r * cos(phi);
    const float p2 = r * sin(phi) * ((r1 < a) ? 1 : v.y);
    const vec3 n = p1 * t1 + p2 * t2 +
        sqrt(max(0, 1 - square(p1) - square(p2))) * v;
    return normalize(vec3(ax * n.x, n.y, ay * n.z));
}

vec3 sample_gtr1(const in float rough) {
    const float r0 = rand_01();
    const float r1 = rand_01();
    const float a2 = sqaure(max(0.001, rough));
    const float phi = r0 * TWO_PI;
    const float cos_theta = sqrt((1 - pow(a2, 1 - r1)) / (1 - a2));
    const float sin_theta = sqrt(1 - min(1.0, square(cos_theta)));
    return vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
}
