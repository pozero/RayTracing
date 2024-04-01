
// Random generator stolen from https://github.com/grigoryoskin/vulkan-compute-ray-tracing/blob/master/resources/shaders/source/include/random.glsl

uint random_seed_step(const in uint r) {
    return r * 747796405 + 1;
}

void random_seed_hash_combine(const in uint v) {
    seed = v + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

float rand_01() {
    seed = random_seed_step(seed);
    uint word = ((seed >> ((seed >> 28) + 4)) ^ seed) * 277803737;
    word = (word >> 22) ^ word;
    return float(word) / 4294967295.0f;
}

float rand_interval(const in float min,
                    const in float max) {
    return min + (max - min) * rand_01();
}

vec3 rand_vector() {
    return vec3(rand_01(), rand_01(), rand_01());
}

vec3 rand_vector_in_unit_sphere() {
    const float r1 = rand_01();
    const float r2 = rand_01();
    const float r3 = rand_01();
    const float phi = 2.0 * PI * r1;
    const float cos_theta = 1 - 2 * r2;
    const float sin_theta = 2 * sqrt(r2 * (1 - r2));
    return r3 * vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

vec3 rand_vector_on_unit_sphere() {
    const float r1 = rand_01();
    const float r2 = rand_01();
    const float phi = 2.0 * PI * r1;
    const float cos_theta = 1 - 2 * r2;
    const float sin_theta = 2 * sqrt(r2 * (1 - r2));
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

vec3 rand_vector_on_hemisphere(const in vec3 normal) {
    const vec3 random_unit_vector = rand_vector_on_unit_sphere();
    return dot(random_unit_vector, normal) <= 0.0 ? -random_unit_vector : 
                                                    random_unit_vector;
}
