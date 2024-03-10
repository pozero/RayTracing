#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"
#pragma clang diagnostic pop

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
    camera.v = glm::cross(camera.u, camera.w);
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

void rotate_camera(
    camera& camera, float cursor_x, float cursor_y, bool holding) {
    static float last_cursor_x = cursor_x;
    static float last_cursor_y = cursor_y;
    if (holding) {
        float const offset_x = last_cursor_x - cursor_x;
        float const offset_y = last_cursor_y - cursor_y;
        camera.yaw += camera.sensitivity * offset_x;
        float const updated_pitch =
            camera.pitch + camera.sensitivity * offset_y;
        if (updated_pitch > -0.5f * glm::pi<float>() &&
            updated_pitch < 0.5f * glm::pi<float>()) {
            camera.pitch = updated_pitch;
        }
        camera.w[0] = std::cos(camera.yaw) * std::cos(camera.pitch);
        camera.w[1] = std::sin(camera.pitch);
        camera.w[2] = std::sin(camera.yaw) * std::cos(camera.pitch);
        camera.u =
            glm::normalize(glm::cross(glm::vec3{0.0f, 1.0f, 0.0f}, camera.w));
        camera.v = glm::cross(camera.u, camera.w);
        camera.dirty = true;
    }
    last_cursor_x = cursor_x;
    last_cursor_y = cursor_y;
}

void move_camera(
    camera& camera, float delta_time, float along_minus_z, float along_x) {
    camera.position += camera.velocity * delta_time *
                       (along_minus_z * -(camera.w) + along_x * camera.u);
    camera.dirty = true;
}

glsl_raytracer_camera get_glsl_raytracer_camera(
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
    float const accumulated_scalar =
        1.0f / static_cast<float>(camera.frame_counter);
    return glsl_raytracer_camera{
        pixel_delta_u,
        pixel_delta_v,
        upper_left_pixel,
        camera.position,
        accumulated_scalar,
    };
}

glm::mat4 get_glsl_render_camera(
    camera const& camera, uint32_t frame_width, uint32_t frame_height) {
    glm::mat4 const view =
        glm::lookAt(camera.position, camera.position - camera.w, -camera.v);
    glm::mat4 const proj =
        glm::perspectiveZO(glm::radians(camera.vertical_field_of_view),
            float(frame_width) / float(frame_height), 0.1f, 100.0f);
    return proj * view;
}

void update_camera(GLFWwindow* window, camera& camera, float delta_time) {
    bool const mouse_left_button_clicked =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    double cursor_x = 0.0;
    double cursor_y = 0.0;
    glfwGetCursorPos(window, &cursor_x, &cursor_y);
    rotate_camera(camera, static_cast<float>(cursor_x),
        static_cast<float>(cursor_y), mouse_left_button_clicked);
    float along_minus_z = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        along_minus_z += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        along_minus_z -= 1.0f;
    }
    float along_x = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        along_x += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        along_x -= 1.0f;
    }
    if (along_minus_z != 0.0f || along_x != 0.0f) {
        move_camera(camera, delta_time, along_minus_z, along_x);
    }
}
