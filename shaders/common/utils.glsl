#define PI 3.1415926

vec3 unpack_vec3(const in float array[3]) {
    return vec3(array[0], array[1], array[2]);
}

float square(const in float val) {
    return val;
}

float pow5(const in float val) {
    const float squared = square(val);
    return square(squared) * val;
}
