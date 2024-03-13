vec3 tone_mapping(const in vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 gamma_correct(const in vec3 color) {
    return pow(color, vec3(1.0 / 2.2));
}
