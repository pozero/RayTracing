#include <iostream>
#include <array>
#include <vector>
#include <fstream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/type_ptr.hpp"
#pragma clang diagnostic pop

struct sphere {
    glm::vec3 center;
    float radius;
};

struct camera {
    glm::vec3 pixel_delta_u;
    glm::vec3 pixel_delta_v;
    glm::vec3 upper_left_pixel;
    glm::vec3 camera_position;
};

static int window_width = 800;
static int window_height = 600;

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

inline void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
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

    int constexpr FRAME_WIDTH = 800;
    int constexpr FRAME_HEIGHT = 600;
    unsigned int frame = 0;
    glGenTextures(1, &frame);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, frame);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, FRAME_WIDTH, FRAME_HEIGHT, 0,
        GL_RGBA, GL_FLOAT, nullptr);
    glBindImageTexture(0, frame, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, frame);

    unsigned int camera_buffer = 0;
    unsigned int constexpr CAMERA_BUFFER_SIZE = 4 * sizeof(glm::vec3);
    glGenBuffers(1, &camera_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, camera_buffer);
    glBufferData(
        GL_UNIFORM_BUFFER, CAMERA_BUFFER_SIZE, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(
        GL_UNIFORM_BUFFER, 0, camera_buffer, 0, CAMERA_BUFFER_SIZE);

    std::array spheres{
        sphere{   glm::vec3{0.0f, 0.0f, -1.0f},   0.5f},
        sphere{glm::vec3{0.0f, -100.5f, -1.0f}, 100.0f},
    };
    unsigned int constexpr SPHERE_BUFFER_SIZE =
        static_cast<unsigned int>(sizeof(sphere) * spheres.size());
    unsigned int sphere_buffer = 0;
    glGenBuffers(1, &sphere_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, sphere_buffer);
    glBufferData(
        GL_UNIFORM_BUFFER, SPHERE_BUFFER_SIZE, spheres.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(
        GL_UNIFORM_BUFFER, 1, sphere_buffer, 0, SPHERE_BUFFER_SIZE);

    int constexpr SAMPLE_PER_PIXEL = 6;
    float constexpr focal_length = 1.0f;
    float constexpr viewport_height = 2.0f;
    float constexpr viewport_width =
        viewport_height *
        (static_cast<float>(FRAME_WIDTH) / static_cast<float>(FRAME_HEIGHT));
    glm::vec3 const camera_position{0.0, 0.0, 0.0};

    unsigned int raytracer_program = glCreateProgram();
    {
        unsigned int const raytracer =
            load_spirv<1>("shaders/raytracer.comp.spv", GL_COMPUTE_SHADER, {0u},
                {static_cast<unsigned int>(spheres.size())});
        glAttachShader(raytracer_program, raytracer);
        glLinkProgram(raytracer_program);
        check_link_error(raytracer_program);
        glDeleteShader(raytracer);
    }

    unsigned int rect_program = glCreateProgram();
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

    while (glfwWindowShouldClose(window) == 0) {
        processInput(window);

        glm::vec3 const viewport_u{viewport_width, 0.0f, 0.0f};
        glm::vec3 const viewport_v{0.0f, -viewport_height, 0.0f};
        glm::vec3 const pixel_delta_u =
            viewport_u / static_cast<float>(FRAME_WIDTH);
        glm::vec3 const pixel_delta_v =
            viewport_v / static_cast<float>(FRAME_HEIGHT);
        glm::vec3 const viewport_upper_left =
            camera_position - glm::vec3{0.0f, 0.0f, focal_length} -
            0.5f * (viewport_u + viewport_v);
        glm::vec3 const upper_left_pixel =
            viewport_upper_left + 0.5f * (pixel_delta_v + pixel_delta_u);
        camera const camera{
            pixel_delta_u,
            pixel_delta_v,
            upper_left_pixel,
            camera_position,
        };
        glBindBuffer(GL_UNIFORM_BUFFER, camera_buffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camera), &camera);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glClearTexImage(frame, 0, GL_RGBA, GL_FLOAT, nullptr);
        glUseProgram(raytracer_program);
        glDispatchCompute(FRAME_WIDTH, FRAME_HEIGHT, SAMPLE_PER_PIXEL);

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

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
