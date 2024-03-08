#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/common.hpp"
#include "glm/glm.hpp"
#pragma clang diagnostic pop

struct glsl_raytracer_camera {
    alignas(sizeof(glm::vec4)) glm::vec3 pixel_delta_u;
    alignas(sizeof(glm::vec4)) glm::vec3 pixel_delta_v;
    alignas(sizeof(glm::vec4)) glm::vec3 upper_left_pixel;
    alignas(sizeof(glm::vec4)) glm::vec3 camera_position;
    float accumulated_scalar;
};

struct raytracer_camera {
    glm::vec3 position;
    glm::vec3 lookat;
    glm::vec3 w;
    glm::vec3 u;
    glm::vec3 v;

    uint64_t frame_counter = 1;
    uint32_t accumulation_idx = 0;

    float vertical_field_of_view;
    float focal_length;
    float viewport_height;
    float viewport_width;

    float pitch;
    float yaw;

    float sensitivity = 0.01f;
    float velocity = 1.0f;

    bool dirty = false;
};

raytracer_camera create_raytracer_camera(glm::vec3 const& lookfrom,
    glm::vec3 const& lookat, float vfov, uint32_t frame_width,
    uint32_t frame_height);

void raytracer_camera_rotate(
    raytracer_camera& camera, float cursor_x, float cursor_y, bool holding);

void raytracer_camera_move(raytracer_camera& camera, float delta_time,
    float along_minus_z, float along_x);

glsl_raytracer_camera get_glsl_raytracer_camera(raytracer_camera const& camera,
    uint32_t frame_width, uint32_t frame_height);

void update_raytraver_camera(
    struct GLFWwindow* window, raytracer_camera& camera, float delta_time);
