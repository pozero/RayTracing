float power_heuristic(const in float a, const in float b) {
    const float a2 = a * a;
    return a2 / (a2 + b * b);
}

vec3 uniform_sample_sphere() {
    const float r0 = rand_01();
    const float r1 = rand_01();
    const float cos_theta = 1 - 2.0 * r0;
    const float sin_theta = sqrt(max(0.0, 1 - cos_theta * cos_theta));
    const float phi = 2.0 * r1;
    return vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
}

vec3 uniform_sample_hemisphere(const in vec3 vec) {
    const vec3 r = uniform_sample_sphere();
    return dot(r, vec) < 0 ? -r : r;
}

vec3 uniform_sample_triangle() {
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

vec3 cosine_sample_hemisphere() {
    const float r1 = rand_01();
    const float r2 = rand_01();
    const float r = sqrt(r1);
    const float phi = TWO_PI * r2;
    const float x = r * cos(phi);
    const float y = r * sin(phi);
    const float z = sqrt(max(0.0, 1.0 - x * x - y * y));
    return vec3(x, y, z);
}

vec3 sample_ggx_vndf(const in vec3 V, const in float ax, const in float ay) {
    const float r1 = rand_01();
    const float r2 = rand_01();
    const vec3 Vh = normalize(vec3(ax * V.x, ay * V.y, V.z));
    const float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    const vec3 T1 =
        lensq > 0 ? vec3(-Vh.y, Vh.x, 0) * inversesqrt(lensq) : vec3(1, 0, 0);
    const vec3 T2 = cross(Vh, T1);
    const float r = sqrt(r1);
    const float phi = 2.0 * PI * r2;
    const float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    const float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;
    const vec3 Nh =
        t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;
    return normalize(vec3(ax * Nh.x, ay * Nh.y, max(0.0, Nh.z)));
}

vec3 sample_gtr1(const in float roughness) {
    const float r1 = rand_01();
    const float r2 = rand_01();
    const float a = max(0.001, roughness);
    const float a2 = a * a;
    const float phi = r1 * TWO_PI;
    const float cos_theta = sqrt((1.0 - pow(a2, 1.0 - r2)) / (1.0 - a2));
    const float sin_theta =
        clamp(sqrt(1.0 - (cos_theta * cos_theta)), 0.0, 1.0);
    const float sin_phi = sin(phi);
    const float cos_phi = cos(phi);
    return vec3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
}
