float rand_01(const in vec2 co){
    return fract(sin(dot(co,vec2(12.9898,78.233))) * 43758.5453);
}

float rand_interval(const in vec2 co,
                    const in float min,
                    const in float max) {
    return min + (max - min) * rand_01(co);
}

vec3 rand_vector(const in vec3 v1,
                 const in vec3 v2) {
    return vec3(
        rand_01(v1.xy),
        rand_01(vec2(v1.z, v2.x)),
        rand_01(v2.yz)
    );
}

vec3 rand_vector_in_unit_sphere(const in vec3 v1,
                                const in vec3 v2) {
    const vec3 seed = rand_vector(v1, v2);
    const float theta = 2.0 * PI * seed.x;
    const float phi = 2.0 * PI * seed.y;
    return seed.z * vec3(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));
}

vec3 rand_vector_on_unit_sphere(const in vec2 v1,
                                const in vec2 v2) {
    const float theta = rand_interval(v1, 0.0, 2 * PI);
    const float phi = rand_interval(v2, 0.0, 2 * PI);
    return vec3(cos(theta) * sin(phi), 
                sin(theta) * sin(phi), 
                cos(phi));
}

vec3 rand_vector_on_hemisphere(const in vec2 v1,
                               const in vec2 v2,
                               const in vec3 normal) {
    const vec3 random_unit_vector = rand_vector_on_unit_sphere(v1, v2);
    return dot(random_unit_vector, normal) <= 0.0 ? -random_unit_vector : 
                                                    random_unit_vector;
}

