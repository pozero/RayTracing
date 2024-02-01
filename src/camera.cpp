#include "camera.h"

camera create_camera(glm::vec3 const& lookfrom, glm::vec3 const& lookat,
    float vfov, uint32_t frame_width, uint32_t frame_height) {
    camera camera{};
    camera.position = lookfrom;
    camera.lookat = lookat;
    camera.vertical_field_of_view = vfov;
    camera.w = glm::normalize(camera.position - camera.lookat);
    camera.u =
        glm::normalize(glm::cross(glm::vec3{0.0f, 1.0f, 0.0f}, camera.w));
    camera.v = glm::cross(camera.w, camera.u);
    camera.focal_length = 1.0f;
    camera.viewport_height =
        2 * glm::tan(glm::radians(camera.vertical_field_of_view) / 2) *
        camera.focal_length;
    camera.viewport_width =
        camera.viewport_height * ((float) frame_width / (float) frame_height);
    camera.pitch = std::asin(camera.w[1]);
    camera.yaw = std::asin(camera.w[2] / std::cos(camera.pitch));
    return camera;
}

void camera_rotate(
    camera& camera, float cursor_x, float cursor_y, bool holding) {
    static float last_cursor_x = cursor_x;
    static float last_cursor_y = cursor_y;
    if (holding) {
        float const offset_x = last_cursor_x - cursor_x;
        float const offset_y = last_cursor_y - cursor_y;
        camera.yaw += camera.sensitivity * offset_x;
        float const updated_pitch =
            camera.pitch + camera.sensitivity * offset_y;
        if (updated_pitch > -89.0f && updated_pitch < 89.0f) {
            camera.pitch = updated_pitch;
        }
        camera.w[0] = std::cos(camera.yaw) * std::cos(camera.pitch);
        camera.w[1] = std::sin(camera.pitch);
        camera.w[2] = std::sin(camera.yaw) * std::cos(camera.pitch);
        camera.u =
            glm::normalize(glm::cross(glm::vec3{0.0f, 1.0f, 0.0f}, camera.w));
        camera.v = glm::cross(camera.w, camera.u);
        camera.dirty = true;
    }
    last_cursor_x = cursor_x;
    last_cursor_y = cursor_y;
}

void camera_move(
    camera& camera, float delta_time, float along_minus_z, float along_x) {
    camera.position += camera.velocity * delta_time *
                       (along_minus_z * -(camera.w) + along_x * camera.u);
    camera.dirty = true;
}

glsl_camera get_glsl_camera(
    camera const& camera, uint32_t frame_width, uint32_t frame_height) {
    glm::vec3 const viewport_u = camera.viewport_width * camera.u;
    glm::vec3 const viewport_v = camera.viewport_height * camera.v;
    glm::vec3 const pixel_delta_u = viewport_u / (float) frame_width;
    glm::vec3 const pixel_delta_v = viewport_v / (float) frame_height;
    glm::vec3 const viewport_upper_left = camera.position -
                                          camera.focal_length * camera.w -
                                          0.5f * (viewport_u + viewport_v);
    glm::vec3 const upper_left_pixel =
        viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);
    float accumulated_scalar = 1.0f / static_cast<float>(camera.frame_counter);
    return glsl_camera{
        pixel_delta_u,
        pixel_delta_v,
        upper_left_pixel,
        camera.position,
        accumulated_scalar,
    };
}
