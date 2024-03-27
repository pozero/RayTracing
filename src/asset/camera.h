#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/common.hpp"
#include "glm/glm.hpp"
#pragma clang diagnostic pop

struct glsl_raytracer_camera {
    glm::vec3 pixel_delta_u;
    glm::vec3 pixel_delta_v;
    glm::vec3 upper_left_pixel;
    glm::vec3 camera_position;
    float accumulated_scalar;
};

struct camera {
    glm::vec3 position;
    glm::vec3 w;
    glm::vec3 u;
    glm::vec3 v;

    float pitch;
    float yaw;

    float sensitivity = 0.01f;
    float velocity = 1.0f;
    float fov = 45.0f;

    bool dirty = false;
};

camera create_camera(
    glm::vec3 const& lookfrom, glm::vec3 const& lookat, float fov);

void rotate_camera(
    camera& camera, float cursor_x, float cursor_y, bool holding);

void move_camera(
    camera& camera, float delta_time, float along_w, float along_u);

glsl_raytracer_camera get_glsl_raytracer_camera(
    camera const& camera, uint32_t frame_width, uint32_t frame_height);

glm::mat4 get_glsl_rasterizer_camera(
    camera const& camera, uint32_t frame_width, uint32_t frame_height);

glm::mat4 get_glsl_rasterizer_camera_for_environment_map(
    const camera& camera, uint32_t frame_width, uint32_t frame_height);

void update_camera(struct GLFWwindow* window, camera& camera, float delta_time);
