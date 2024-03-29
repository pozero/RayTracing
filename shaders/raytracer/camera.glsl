struct camera_t {
    vec3 pixel_delta_u;
    vec3 pixel_delta_v;
    vec3 upper_left_pixel;
    vec3 position;
};

camera_t unpack_camera(const in float packed_camera[12]) {
    const vec3 pixel_delta_u    = vec3(packed_camera[0],
                                       packed_camera[1],
                                       packed_camera[2]);
    const vec3 pixel_delta_v    = vec3(packed_camera[3],
                                       packed_camera[4],
                                       packed_camera[5]);
    const vec3 upper_left_pixel = vec3(packed_camera[6],
                                       packed_camera[7],
                                       packed_camera[8]);
    const vec3 position         = vec3(packed_camera[9],
                                       packed_camera[10],
                                       packed_camera[11]);
    return camera_t(pixel_delta_u, pixel_delta_v, upper_left_pixel, position);
}
