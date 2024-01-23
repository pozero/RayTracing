#include <iostream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#pragma clang diagnostic pop

#include "shader.h"

static int window_width = 800;
static int window_height = 600;

inline std::string get_path(std::string_view path) {
    return std::string{ROOT_PATH} + path.data();
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

    int constexpr FRAME_WIDTH = 512;
    int constexpr FRAME_HEIGHT = 512;
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

    float constexpr focal_length = 1.0f;
    float constexpr viewport_height = 2.0f;
    float constexpr viewport_width =
        viewport_height *
        (static_cast<float>(FRAME_WIDTH) / static_cast<float>(FRAME_HEIGHT));
    glm::vec3 const camera_position{0.0, 0.0, 0.0};

    {
        Program comp{get_path("/shaders/colorful.comp")};
        comp.set_int("img_output", 0);
        Shader rect_vert{get_path("/shaders/rect.vert"), GL_VERTEX_SHADER};
        Shader rect_frag{get_path("/shaders/rect.frag"), GL_FRAGMENT_SHADER};
        Program rect{rect_vert, rect_frag};
        rect.set_int("frame", 0);

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

            comp.use();
            comp.set_vec3("upper_left_pixel", upper_left_pixel);
            comp.set_vec3("pixel_delta_u", pixel_delta_u);
            comp.set_vec3("pixel_delta_v", pixel_delta_v);
            comp.set_vec3("camera_position", camera_position);
            glDispatchCompute(FRAME_WIDTH, FRAME_HEIGHT, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            rect.use();
            GLuint emptyVAO = 0;
            glGenVertexArrays(1, &emptyVAO);
            glBindVertexArray(emptyVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
