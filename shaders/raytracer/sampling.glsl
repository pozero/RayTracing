float power_heuristic(const in float a,
                      const in float b) {
    const float a_squared = a * a;
    return a_squared / (a_squared + b * b);
}

vec3 uniform_sampling_triangle() {
    const float r1 = rand_01();
    const float r2 = rand_01();
    const float sqrt_r1 = sqrt(r1);
    const float b0 = 1 - sqrt_r1;
    const float b1 = r2 * sqrt_r1;
    const float b2 = 1 - b0 - b1;
    return vec3(b0, b1, b2);
}
