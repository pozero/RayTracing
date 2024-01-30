#include <iostream>
#include <array>
#include <vector>
#include <fstream>
#include <random>
#include <utility>
#include <limits>
#include <ctime>
#include <iomanip>
#include <sstream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image_write.h"
#pragma clang diagnostic pop

#define MATERIAL_LAMBERTIAN 0
#define MATERIAL_METAL 1
#define MATERIAL_DIELECTRIC 2

#pragma clang diagnostic ignored "-Wpadded"
struct glsl_material_t {
    int type;
    alignas(sizeof(glm::vec4)) glm::vec3 albedo;
    float fuzz;
    float refraction_index;
};

inline auto rounded_up_to_power_2(auto val) {
    --val;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    ++val;
    return val;
}

inline glsl_material_t material_lambertian(glm::vec3 const &albedo) {
    return glsl_material_t{MATERIAL_LAMBERTIAN, albedo, 0.0f, 0.0f};
}

inline glsl_material_t material_metal(glm::vec3 const &albedo, float fuzz) {
    return glsl_material_t{MATERIAL_METAL, albedo, fuzz, 0.0f};
}

inline glsl_material_t material_dielectric(float refraction_index) {
    return glsl_material_t{MATERIAL_DIELECTRIC, {}, 0.0f, refraction_index};
}

struct glsl_aabb_t {
    glm::vec2 x_interval;
    glm::vec2 y_interval;
    glm::vec2 z_interval;
};

inline glsl_aabb_t empty_aabb() {
    glm::vec2 const empty_interval{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::min(),
    };
    return glsl_aabb_t{
        empty_interval,
        empty_interval,
        empty_interval,
    };
}

inline float aabb_volume(glsl_aabb_t const &aabb) {
    return (aabb.x_interval[1] - aabb.x_interval[0]) *
           (aabb.y_interval[1] - aabb.y_interval[0]) *
           (aabb.z_interval[1] - aabb.z_interval[0]);
}

struct glsl_uniform_grid_t {
    alignas(sizeof(glm::vec4)) glm::vec3 starting_point;
    alignas(sizeof(glm::vec4)) glm::vec3 grid_size;
    alignas(sizeof(glm::vec4)) glm::vec3 inv_grid_size;
};

struct glsl_acceleration_structure_t {
    glsl_uniform_grid_t uniform_grid;
    glsl_aabb_t world_aabb;
};

struct glsl_sphere_t {
    glm::vec3 center;
    float radius;
    glsl_material_t material;
    glsl_aabb_t aabb;
};

inline glsl_sphere_t create_sphere(
    glm::vec3 const &center, float radius, glsl_material_t const &material) {
    glm::vec2 const epsilon{-1.0f, 1.0f};
    glm::vec2 const x_interval =
        glm::vec2{center[0] - std::abs(radius), center[0] + std::abs(radius)} +
        epsilon;
    glm::vec2 const y_interval =
        glm::vec2{center[1] - std::abs(radius), center[1] + std::abs(radius)} +
        epsilon;
    glm::vec2 const z_interval =
        glm::vec2{center[2] - std::abs(radius), center[2] + std::abs(radius)} +
        epsilon;
    glsl_aabb_t const aabb{
        x_interval,
        y_interval,
        z_interval,
    };
    return glsl_sphere_t{
        .center = center,
        .radius = radius,
        .material = material,
        .aabb = aabb,
    };
}

inline glsl_aabb_t aabb_union(
    glsl_aabb_t const &box_a, glsl_aabb_t const &box_b) {
    return glsl_aabb_t{
        glm::vec2{std::min(box_a.x_interval[0], box_b.x_interval[0]),
                  std::max(box_a.x_interval[1], box_b.x_interval[1])},
        glm::vec2{std::min(box_a.y_interval[0], box_b.y_interval[0]),
                  std::max(box_a.y_interval[1], box_b.y_interval[1])},
        glm::vec2{std::min(box_a.z_interval[0], box_b.z_interval[0]),
                  std::max(box_a.z_interval[1], box_b.z_interval[1])},
    };
}

inline glsl_aabb_t get_world_aabb(std::vector<glsl_sphere_t> const &spheres) {
    glsl_aabb_t world_aabb = empty_aabb();
    for (glsl_sphere_t const &sphere : spheres) {
        world_aabb = aabb_union(world_aabb, sphere.aabb);
    }
    return world_aabb;
}

inline glsl_uniform_grid_t create_uniform_grid(glsl_aabb_t const &world_aabb,
    uint32_t grid_along_x, uint32_t grid_along_y, uint32_t grid_along_z) {
    glm::vec3 const mu{1.0f, 1.0f, 1.0f};
    glm::vec3 const starting_point =
        glm::vec3{world_aabb.x_interval[0], world_aabb.y_interval[0],
            world_aabb.z_interval[0]} -
        mu;
    glm::vec3 const inv_count =
        glm::vec3{1.0f, 1.0f, 1.0f} /
        glm::vec3{grid_along_x, grid_along_y, grid_along_z};
    glm::vec3 const world_aabb_size =
        glm::vec3{world_aabb.x_interval[1] - world_aabb.x_interval[0],
            world_aabb.y_interval[1] - world_aabb.y_interval[0],
            world_aabb.z_interval[1] - world_aabb.z_interval[0]} +
        2.0f * mu;
    glm::vec3 const grid_size = inv_count * world_aabb_size;
    glm::vec3 const inv_grid_size = glm::vec3{1.0f, 1.0f, 1.0f} / grid_size;
    return glsl_uniform_grid_t{starting_point, grid_size, inv_grid_size};
}

struct glsl_camera_t {
    alignas(sizeof(glm::vec4)) glm::vec3 pixel_delta_u;
    alignas(sizeof(glm::vec4)) glm::vec3 pixel_delta_v;
    alignas(sizeof(glm::vec4)) glm::vec3 upper_left_pixel;
    alignas(sizeof(glm::vec4)) glm::vec3 camera_position;
    float accumulated_scalar;
};

#define LOCAL_ELEMENTS_COUNT 16
#define LOCAL_BITONIC_MERGE_SORT 0
#define LOCAL_DISPERSE 1
#define BIG_FLIP 2
#define BIG_DISPERSE 3

struct glsl_bitonic_sort_parameter_t {
    uint32_t h;
    uint32_t algorithm;
};

int constexpr FRAME_WIDTH = 1200;
int constexpr FRAME_HEIGHT = 500;

struct camera_t {
    glm::vec3 position{13.0f, 2.0f, 3.0f};
    glm::vec3 lookat{0.0f, 0.0f, 0.0f};
    glm::vec3 relative_up{0.0f, 1.0f, 0.0f};
    glm::vec3 w;
    glm::vec3 u;
    glm::vec3 v;

    uint64_t frame_counter = 1;

    float vertical_field_of_view = 20.0f;
    float focal_length;
    float viewport_height;
    float viewport_width;

    float pitch;
    float yaw;

    float sensitivity = 0.01f;
    float velocity = 1.0f;

    bool dirty = false;
};

inline camera_t create_camera() {
    camera_t camera{};
    camera.w = glm::normalize(camera.position - camera.lookat);
    camera.u = glm::normalize(glm::cross(camera.relative_up, camera.w));
    camera.v = glm::cross(camera.w, camera.u);
    camera.focal_length = 1.0f;
    camera.viewport_height =
        2 * glm::tan(glm::radians(camera.vertical_field_of_view) / 2) *
        camera.focal_length;
    camera.viewport_width =
        camera.viewport_height *
        (static_cast<float>(FRAME_WIDTH) / static_cast<float>(FRAME_HEIGHT));
    camera.pitch = std::asin(camera.w[1]);
    camera.yaw = std::asin(camera.w[2] / std::cos(camera.pitch));
    return camera;
}

inline void camera_rotate(
    camera_t &camera, float cursor_x, float cursor_y, bool holding) {
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
        camera.u = glm::normalize(glm::cross(camera.relative_up, camera.w));
        camera.v = glm::cross(camera.w, camera.u);
        camera.dirty = true;
    }
    last_cursor_x = cursor_x;
    last_cursor_y = cursor_y;
}

inline void camera_move(
    camera_t &camera, float delta_time, float along_minus_z, float along_x) {
    camera.position += camera.velocity * delta_time *
                       (along_minus_z * -(camera.w) + along_x * camera.u);
    camera.dirty = true;
}

inline glsl_camera_t get_glsl_camera(camera_t const &camera) {
    glm::vec3 const viewport_u = camera.viewport_width * camera.u;
    glm::vec3 const viewport_v = camera.viewport_height * camera.v;
    glm::vec3 const pixel_delta_u =
        viewport_u / static_cast<float>(FRAME_WIDTH);
    glm::vec3 const pixel_delta_v =
        viewport_v / static_cast<float>(FRAME_HEIGHT);
    glm::vec3 const viewport_upper_left = camera.position -
                                          camera.focal_length * camera.w -
                                          0.5f * (viewport_u + viewport_v);
    glm::vec3 const upper_left_pixel =
        viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);
    float accumulated_scalar = 1.0f / static_cast<float>(camera.frame_counter);
    return glsl_camera_t{
        pixel_delta_u,
        pixel_delta_v,
        upper_left_pixel,
        camera.position,
        accumulated_scalar,
    };
}

static int window_width = FRAME_WIDTH;
static int window_height = FRAME_HEIGHT;

inline std::vector<char> read_binary_whole(std::string_view path) {
    std::ifstream ifs{path.data(), std::ios::binary | std::ios::ate};
    if (!ifs.is_open()) {
        std::cerr << "Can't find file " << path.data() << '\n';
        std::terminate();
    }
    std::ifstream::pos_type pos = ifs.tellg();
    if (pos == 0) {
        return std::vector<char>{};
    }
    std::vector<char> result(static_cast<size_t>(pos), 0);
    ifs.seekg(0, std::ios::beg);
    ifs.read(result.data(), pos);
    return result;
}

inline unsigned int load_spirv(std::string_view path, GLenum type) {
    std::vector<char> const bytecode = read_binary_whole(path);
    unsigned int const shader = glCreateShader(type);
    glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, bytecode.data(),
        static_cast<int>(bytecode.size()));
    glSpecializeShader(shader, "main", 0, nullptr, nullptr);
    int success = 0;
    size_t constexpr BUF_SIZE = 1024;
    std::array<char, BUF_SIZE> buf{};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == 0) {
        glGetShaderInfoLog(shader, buf.size(), nullptr, buf.data());
        std::cerr << path.data() << ' ' << "Compilation error: " << buf.data()
                  << '\n';
        std::terminate();
    }
    return shader;
}

template <unsigned int SPECIALIZATION_CONSTANT_COUNT>
unsigned int load_spirv(std::string_view path, GLenum type,
    std::array<unsigned int, SPECIALIZATION_CONSTANT_COUNT> const &indices,
    std::array<unsigned int, SPECIALIZATION_CONSTANT_COUNT> const &values) {
    std::vector<char> const bytecode = read_binary_whole(path);
    unsigned shader = glCreateShader(type);
    glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, bytecode.data(),
        static_cast<int>(bytecode.size()));
    glSpecializeShader(shader, "main", SPECIALIZATION_CONSTANT_COUNT,
        indices.data(), values.data());
    int success = 0;
    size_t constexpr BUF_SIZE = 1024;
    std::array<char, BUF_SIZE> buf{};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == 0) {
        glGetShaderInfoLog(shader, buf.size(), nullptr, buf.data());
        std::cerr << path.data() << ' ' << "Compilation error: " << buf.data()
                  << '\n';
        std::terminate();
    }
    return shader;
}

inline void check_link_error(unsigned program) {
    int success = 0;
    size_t constexpr BUF_SIZE = 1024;
    std::array<char, BUF_SIZE> buf{};
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == 0) {
        glGetProgramInfoLog(program, buf.size(), nullptr, buf.data());
        std::cerr << "Link error: " << buf.data() << '\n';
    }
}

inline void framebuffer_size_callback(
    [[maybe_unused]] GLFWwindow *window, int width, int height) {
    window_width = width;
    window_height = height;
    glViewport(0, 0, width, height);
}

inline void process_input(
    GLFWwindow *window, camera_t &camera, float delta_time) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
    }
    bool const mouse_left_button_clicked =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    double cursor_x = 0.0;
    double cursor_y = 0.0;
    glfwGetCursorPos(window, &cursor_x, &cursor_y);
    camera_rotate(camera, static_cast<float>(cursor_x),
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
        camera_move(camera, delta_time, along_minus_z, along_x);
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(
        window_width, window_height, "RayTracing", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << '\n';
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
    if (gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) == 0) {
#pragma clang diagnostic pop
        std::cout << "Failed to initialize GLAD" << '\n';
        return -1;
    }
    const GLubyte *renderer = glGetString(GL_RENDERER);
    std::clog << renderer << '\n';
    glViewport(0, 0, window_width, window_height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    unsigned int accumulation_buffer = 0;
    glGenTextures(1, &accumulation_buffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, accumulation_buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, FRAME_WIDTH, FRAME_HEIGHT, 0,
        GL_RGBA, GL_FLOAT, nullptr);
    glBindImageTexture(
        0, accumulation_buffer, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glActiveTexture(GL_TEXTURE0);

    unsigned int camera_buffer = 0;
    glGenBuffers(1, &camera_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, camera_buffer);
    glBufferData(
        GL_UNIFORM_BUFFER, sizeof(glsl_camera_t), nullptr, GL_STATIC_READ);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(
        GL_UNIFORM_BUFFER, 0, camera_buffer, 0, sizeof(glsl_camera_t));

    std::vector<glsl_sphere_t> spheres{};

    // spheres.push_back(create_sphere(glm::vec3{0.0f, 0.0f, -1.0f}, 0.5f,
    //     material_lambertian(glm::vec3{0.1f, 0.2f, 0.5f})));
    // spheres.push_back(create_sphere(
    //     glm::vec3{-1.0f, 0.0f, -1.0f}, 0.5f, material_dielectric(1.5f)));
    // spheres.push_back(create_sphere(
    //     glm::vec3{-1.0f, 0.0f, -1.0f}, -0.4f, material_dielectric(1.5f)));
    // spheres.push_back(create_sphere(glm::vec3{1.0f, 0.0f, -1.0f}, 0.5f,
    //     material_metal(glm::vec3{0.8f, 0.6f, 0.2f}, 0.3f)));
    // spheres.push_back(create_sphere(glm::vec3{0.0f, -100.5f, -1.0f}, 100.0f,
    //     material_lambertian(glm::vec3{0.8f, 0.8f, 0.0f})));

    spheres.push_back(create_sphere(glm::vec3{0.0f, -1000.0f, 0.0f}, 1000.0f,
        material_lambertian(glm::vec3{0.5f, 0.5f, 0.5f})));
    spheres.push_back(create_sphere(
        glm::vec3{0.0f, 1.0f, 0.0f}, 1.0f, material_dielectric(1.5f)));
    spheres.push_back(create_sphere(glm::vec3{-4.0f, 1.0f, 0.0f}, 1.0f,
        material_lambertian(glm::vec3{0.4f, 0.2f, 0.1f})));
    spheres.push_back(create_sphere(glm::vec3{4.0f, 1.0f, 0.0f}, 1.0f,
        material_metal(glm::vec3{0.7f, 0.6f, 0.5f}, 0.0f)));
    std::random_device dev{};
    std::mt19937 rng{dev()};
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (int a = 0; a < 11; ++a) {
        for (int b = 0; b < 11; ++b) {
            float const choose_material = dist(rng);
            glm::vec3 const center{static_cast<float>(a) + 0.9f * dist(rng),
                0.2f, static_cast<float>(b) + 0.9f * dist(rng)};
            if (glm::distance(center, glm::vec3{4.0f, 0.2f, 0.0f}) > 0.9f) {
                glsl_material_t material{
                    0,
                    glm::vec3{dist(rng), dist(rng), dist(rng)},
                    0.5f * dist(rng),
                    1.5f,
                };
                if (choose_material < 0.8f) {
                    material.type = MATERIAL_LAMBERTIAN;
                } else if (choose_material < 0.95f) {
                    material.type = MATERIAL_METAL;
                } else {
                    material.type = MATERIAL_DIELECTRIC;
                }
                spheres.push_back(create_sphere(center, 0.2f, material));
            }
        }
    }

    unsigned int const sphere_buffer_size =
        static_cast<unsigned int>(sizeof(glsl_sphere_t) * spheres.size());
    unsigned int sphere_buffer = 0;
    glGenBuffers(1, &sphere_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, sphere_buffer);
    glBufferData(
        GL_UNIFORM_BUFFER, sphere_buffer_size, spheres.data(), GL_STATIC_READ);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(
        GL_UNIFORM_BUFFER, 1, sphere_buffer, 0, sphere_buffer_size);

    glsl_aabb_t const world_aabb = get_world_aabb(spheres);
    float const uniform_sparse_parameter = 0.5;
    float const kp_v = glm::pow(
        (uniform_sparse_parameter * static_cast<float>(spheres.size())) /
            aabb_volume(world_aabb),
        1.0f / 3.0f);
    uint32_t const GRID_ALONG_X_AXIS =
        static_cast<uint32_t>(
            (world_aabb.x_interval[1] - world_aabb.x_interval[0]) * kp_v) +
        1;
    uint32_t const GRID_ALONG_Y_AXIS =
        static_cast<uint32_t>(
            (world_aabb.y_interval[1] - world_aabb.y_interval[0]) * kp_v) +
        1;
    uint32_t const GRID_ALONG_Z_AXIS =
        static_cast<uint32_t>(
            (world_aabb.z_interval[1] - world_aabb.z_interval[0]) * kp_v) +
        1;
    if (rounded_up_to_power_2(GRID_ALONG_X_AXIS * GRID_ALONG_Y_AXIS *
                              GRID_ALONG_Z_AXIS * spheres.size()) >=
        std::numeric_limits<uint32_t>::max()) {
        std::cerr << "Too much objects\n";
        std::terminate();
    }
    glsl_uniform_grid_t const uniform_grid = create_uniform_grid(
        world_aabb, GRID_ALONG_X_AXIS, GRID_ALONG_Y_AXIS, GRID_ALONG_Y_AXIS);
    glsl_acceleration_structure_t const acceleration_structure{
        uniform_grid, world_aabb};

    unsigned int acceleration_structure_buffer = 0;
    glGenBuffers(1, &acceleration_structure_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, acceleration_structure_buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glsl_acceleration_structure_t),
        &acceleration_structure, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(GL_UNIFORM_BUFFER, 2, acceleration_structure_buffer, 0,
        sizeof(glsl_acceleration_structure_t));

    unsigned int intersection_start_buffer = 0;
    long const intersection_start_buffer_size =
        static_cast<long>(sizeof(uint32_t) * (spheres.size() + 1));
    long const intersection_count_offset =
        intersection_start_buffer_size - static_cast<long>(sizeof(uint32_t));
    glGenBuffers(1, &intersection_start_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, intersection_start_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, intersection_start_buffer_size,
        nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, intersection_start_buffer, 0,
        intersection_start_buffer_size);

    unsigned int sphere_grid_occupation_buffer = 0;
    long const sphere_grid_occupation_buffer_size =
        2 * static_cast<uint32_t>(sizeof(glm::ivec4) * spheres.size());
    glGenBuffers(1, &sphere_grid_occupation_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphere_grid_occupation_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sphere_grid_occupation_buffer_size,
        nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1,
        sphere_grid_occupation_buffer, 0, sphere_grid_occupation_buffer_size);

    std::array<unsigned int, 2> intersection_record_buffer{};
    int64_t const intersection_record_buffer_max_size = rounded_up_to_power_2(
        GRID_ALONG_X_AXIS * GRID_ALONG_Y_AXIS * GRID_ALONG_Z_AXIS *
        static_cast<uint32_t>(spheres.size() * sizeof(uint32_t)));
    glGenBuffers(2, intersection_record_buffer.data());

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, intersection_record_buffer[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, intersection_record_buffer_max_size,
        nullptr, GL_DYNAMIC_DRAW);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2,
        intersection_record_buffer[0], 0, intersection_record_buffer_max_size);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, intersection_record_buffer[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, intersection_record_buffer_max_size,
        nullptr, GL_DYNAMIC_DRAW);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 3,
        intersection_record_buffer[1], 0, intersection_record_buffer_max_size);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    unsigned int sorting_parameter_buffer = 0;
    glGenBuffers(1, &sorting_parameter_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sorting_parameter_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        sizeof(glsl_bitonic_sort_parameter_t), nullptr, GL_DYNAMIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 4, sorting_parameter_buffer, 0,
        sizeof(glsl_bitonic_sort_parameter_t));

    unsigned int geometry_in_grid_buffer = 0;
    long const geometry_in_grid_buffer_size =
        sizeof(uint32_t) *
        (GRID_ALONG_X_AXIS * GRID_ALONG_Y_AXIS * GRID_ALONG_Z_AXIS + 1);
    glGenBuffers(1, &geometry_in_grid_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, geometry_in_grid_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, geometry_in_grid_buffer_size,
        nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 5, geometry_in_grid_buffer, 0,
        geometry_in_grid_buffer_size);

    camera_t camera = create_camera();

    unsigned int const count_grid_program = glCreateProgram();
    {
        unsigned int const shader = load_spirv<4>("shaders/count_grid.comp.spv",
            GL_COMPUTE_SHADER, {0, 1, 2, 3},
            {static_cast<unsigned int>(spheres.size()), GRID_ALONG_X_AXIS,
                GRID_ALONG_Y_AXIS, GRID_ALONG_Z_AXIS});
        glAttachShader(count_grid_program, shader);
        glLinkProgram(count_grid_program);
        check_link_error(count_grid_program);
        glDeleteShader(shader);
    }

    unsigned int const raytracer_program = glCreateProgram();
    {
        unsigned int const raytracer = load_spirv<4>(
            "shaders/raytracer.comp.spv", GL_COMPUTE_SHADER, {0, 1, 2, 3},
            {static_cast<unsigned int>(spheres.size()), GRID_ALONG_X_AXIS,
                GRID_ALONG_Y_AXIS, GRID_ALONG_Z_AXIS});
        glAttachShader(raytracer_program, raytracer);
        glLinkProgram(raytracer_program);
        check_link_error(raytracer_program);
        glDeleteShader(raytracer);
    }

    unsigned int const rect_program = glCreateProgram();
    {
        unsigned int const vert =
            load_spirv("shaders/rect.vert.spv", GL_VERTEX_SHADER);
        unsigned int const frag =
            load_spirv("shaders/rect.frag.spv", GL_FRAGMENT_SHADER);
        glAttachShader(rect_program, vert);
        glAttachShader(rect_program, frag);
        glLinkProgram(rect_program);
        check_link_error(rect_program);
        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    uint32_t constexpr ZERO = 0;
    uint32_t constexpr MAX = std::numeric_limits<uint32_t>::max();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, intersection_start_buffer);
    glClearBufferData(
        GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &ZERO);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glUseProgram(count_grid_program);
    glDispatchCompute(static_cast<uint32_t>(spheres.size()), 1, 1);

    uint32_t intersection_count = 0;
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, intersection_count_offset,
        sizeof(uint32_t), &intersection_count);
    intersection_count = rounded_up_to_power_2(intersection_count);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    unsigned int const write_grid_geometry_program = glCreateProgram();
    {
        unsigned int const shader =
            load_spirv<5>("shaders/write_grid_geometry.comp.spv",
                GL_COMPUTE_SHADER, {0, 1, 2, 3, 4},
                {static_cast<unsigned int>(spheres.size()), GRID_ALONG_X_AXIS,
                    GRID_ALONG_Y_AXIS, GRID_ALONG_Z_AXIS, intersection_count});
        glAttachShader(write_grid_geometry_program, shader);
        glLinkProgram(write_grid_geometry_program);
        check_link_error(write_grid_geometry_program);
        glDeleteShader(shader);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, intersection_record_buffer[0]);
    glClearBufferData(
        GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &MAX);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, intersection_record_buffer[1]);
    glClearBufferData(
        GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &MAX);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glUseProgram(write_grid_geometry_program);
    glDispatchCompute(static_cast<uint32_t>(spheres.size()), 1, 1);

    uint32_t h = LOCAL_ELEMENTS_COUNT * 2;
    uint32_t const sort_work_group_count = intersection_count / h;
    glsl_bitonic_sort_parameter_t sort_parameter{h, LOCAL_BITONIC_MERGE_SORT};

    unsigned int const sort_grid_geometry_program = glCreateProgram();
    {
        unsigned int const shader = load_spirv<2>(
            "shaders/sort_grid_geometry_by_grid.comp.spv", GL_COMPUTE_SHADER,
            {0, 1}, {intersection_count, LOCAL_ELEMENTS_COUNT});
        glAttachShader(sort_grid_geometry_program, shader);
        glLinkProgram(sort_grid_geometry_program);
        check_link_error(sort_grid_geometry_program);
        glDeleteShader(shader);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sorting_parameter_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
        sizeof(glsl_bitonic_sort_parameter_t), &sort_parameter);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
    glUseProgram(sort_grid_geometry_program);
    glDispatchCompute(sort_work_group_count, 1, 1);
    h *= 2;
    for (; h <= intersection_count; h *= 2) {
        sort_parameter.algorithm = BIG_FLIP;
        sort_parameter.h = h;
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
            sizeof(glsl_bitonic_sort_parameter_t), &sort_parameter);
        glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
        glDispatchCompute(sort_work_group_count, 1, 1);
        for (uint32_t hh = h / 2; hh > 1; hh /= 2) {
            sort_parameter.algorithm =
                hh <= sort_work_group_count * 2 ? LOCAL_DISPERSE : BIG_DISPERSE;
            sort_parameter.h = hh;
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                sizeof(glsl_bitonic_sort_parameter_t), &sort_parameter);
            glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
            glDispatchCompute(sort_work_group_count, 1, 1);
        }
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    unsigned int const count_geometry_program = glCreateProgram();
    {
        unsigned int const shader = load_spirv<4>(
            "shaders/count_geometry.comp.spv", GL_COMPUTE_SHADER, {0, 1, 2, 3},
            {GRID_ALONG_X_AXIS, GRID_ALONG_Y_AXIS, GRID_ALONG_Z_AXIS,
                intersection_count});
        glAttachShader(count_geometry_program, shader);
        glLinkProgram(count_geometry_program);
        check_link_error(count_geometry_program);
        glDeleteShader(shader);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, geometry_in_grid_buffer);
    glClearBufferData(
        GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &ZERO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glUseProgram(count_geometry_program);
    glDispatchCompute(GRID_ALONG_X_AXIS, GRID_ALONG_Y_AXIS, GRID_ALONG_Z_AXIS);

    float last_time = static_cast<float>(glfwGetTime());
    while (glfwWindowShouldClose(window) == 0) {
        float const current_time = static_cast<float>(glfwGetTime());
        float const delta_time = current_time - last_time;
        last_time = current_time;
        process_input(window, camera, delta_time);

        if (camera.dirty) {
            camera.dirty = false;
            camera.frame_counter = 1;
            glClearTexImage(accumulation_buffer, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
        glsl_camera_t const glsl_camera = get_glsl_camera(camera);
        ++camera.frame_counter;
        glBindBuffer(GL_UNIFORM_BUFFER, camera_buffer);
        glBufferSubData(
            GL_UNIFORM_BUFFER, 0, sizeof(glsl_camera), &glsl_camera);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glUseProgram(raytracer_program);
        glDispatchCompute(FRAME_WIDTH, FRAME_HEIGHT, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(rect_program);
        GLuint emptyVAO = 0;
        glGenVertexArrays(1, &emptyVAO);
        glBindVertexArray(emptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::vector<uint8_t> readback(
        static_cast<size_t>(window_width * window_height * 4));
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, window_width, window_height, GL_RGBA, GL_UNSIGNED_BYTE,
        readback.data());
    stbi_flip_vertically_on_write(true);
    auto time = std::time(nullptr);
    auto tm = *std::localtime(&time);
    std::ostringstream image_name{};
    image_name << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    image_name << ".png";
    stbi_write_png(image_name.str().c_str(), window_width, window_height, 4,
        readback.data(), 4 * window_width);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
