#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"
#pragma clang diagnostic pop

#include "camera.h"

inline void get_camera_coord(camera& camera) {
    float const cos_yaw = std::cos(camera.yaw);
    float const sin_yaw = std::sin(camera.yaw);
    float const cos_pitch = std::cos(camera.pitch);
    float const sin_pitch = std::sin(camera.pitch);
    glm::vec3 const unnormalized_front{
        cos_pitch * cos_yaw,
        sin_pitch,
        cos_pitch * sin_yaw,
    };
    camera.front = glm::normalize(unnormalized_front);
    camera.right = glm::normalize(glm::cross(
        camera.front, camera.front.y < 0.99f ? glm::vec3{0.0f, 1.0f, 0.0f} :
                                               glm::vec3{-1.0f, 0.0f, 0.0f}));
    camera.up = glm::normalize(glm::cross(camera.right, camera.front));
}

camera create_camera(
    glm::vec3 const& lookfrom, glm::vec3 const& lookat, float fov) {
    camera camera{};
    camera.position = lookfrom;
    camera.fov = fov;
    camera.front = glm::normalize(lookat - lookfrom);
    camera.pitch = std::asin(camera.front.y);
    camera.yaw = std::atan2(camera.front.z, camera.front.x);
    get_camera_coord(camera);
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
        get_camera_coord(camera);
        camera.dirty = true;
    }
    last_cursor_x = cursor_x;
    last_cursor_y = cursor_y;
}

void move_camera(
    camera& camera, float delta_time, float along_w, float along_u) {
    camera.position += camera.velocity * delta_time *
                       (along_w * camera.front + along_u * camera.right);
    camera.dirty = true;
}

glsl_raytracer_camera get_glsl_raytracer_camera(
    camera const& camera, uint32_t frame_width, uint32_t frame_height) {
    float constexpr FOCAL_LENGTH = 1.0f;
    float const viewport_height =
        2 * glm::tan(glm::radians(camera.fov) / 2) * FOCAL_LENGTH;
    float const viewport_width =
        viewport_height * ((float) frame_width / (float) frame_height);
    glm::vec3 const viewport_u = viewport_width * camera.right;
    glm::vec3 const viewport_v = viewport_height * -camera.up;
    glm::vec3 const pixel_delta_u = viewport_u / (float) frame_width;
    glm::vec3 const pixel_delta_v = viewport_v / (float) frame_height;
    glm::vec3 const viewport_upper_left = camera.position +
                                          FOCAL_LENGTH * camera.front -
                                          0.5f * (viewport_u + viewport_v);
    glm::vec3 const upper_left_pixel =
        viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);
    return glsl_raytracer_camera{
        pixel_delta_u,
        pixel_delta_v,
        upper_left_pixel,
        camera.position,
    };
}

inline glm::mat4 get_glsl_render_camera_view(camera const& camera) {
    glm::mat4 view_mat{1.0f};
    view_mat[0][0] = camera.right.x;
    view_mat[1][0] = camera.right.y;
    view_mat[2][0] = camera.right.z;
    view_mat[0][1] = -camera.up.x;
    view_mat[1][1] = -camera.up.y;
    view_mat[2][1] = -camera.up.z;
    view_mat[0][2] = -camera.front.x;
    view_mat[1][2] = -camera.front.y;
    view_mat[2][2] = -camera.front.z;
    view_mat[3][0] = -glm::dot(camera.right, camera.position);
    view_mat[3][1] = glm::dot(camera.up, camera.position);
    view_mat[3][2] = glm::dot(camera.front, camera.position);
    return view_mat;
}

inline glm::mat4 get_glsl_render_camera_proj(
    camera const& camera, uint32_t frame_width, uint32_t frame_height) {
    return glm::perspectiveZO(glm::radians(camera.fov),
        float(frame_width) / float(frame_height), 0.1f, 100.0f);
}

glm::mat4 get_glsl_rasterizer_camera(
    camera const& camera, uint32_t frame_width, uint32_t frame_height) {
    glm::mat4 const view = get_glsl_render_camera_view(camera);
    glm::mat4 const proj =
        get_glsl_render_camera_proj(camera, frame_width, frame_height);
    return proj * view;
}

glm::mat4 get_glsl_rasterizer_camera_for_environment_map(
    const camera& camera, uint32_t frame_width, uint32_t frame_height) {
    glm::mat4 const view =
        glm::mat4{glm::mat3{get_glsl_render_camera_view(camera)}};
    glm::mat4 const proj =
        get_glsl_render_camera_proj(camera, frame_width, frame_height);
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
    float along_minus_w = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        along_minus_w += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        along_minus_w -= 1.0f;
    }
    float along_u = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        along_u += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        along_u -= 1.0f;
    }
    if (along_minus_w != 0.0f || along_u != 0.0f) {
        move_camera(camera, delta_time, along_minus_w, along_u);
    }
}
