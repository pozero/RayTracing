#include <iostream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#pragma clang diagnostic pop

#include "shader.h"

inline std::string get_path(std::string_view path) {
    return std::string{ROOT_PATH} + path.data();
}

inline void framebuffer_size_callback(
    [[maybe_unused]] GLFWwindow *window, int width, int height) {
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
    int constexpr WINDOW_WIDTH = 800;
    int constexpr WINDOW_HEIGHT = 600;
    GLFWwindow *window = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT, "RayTracing", nullptr, nullptr);
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
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
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

    {
        Program comp{get_path("/shaders/colorful.comp")};
        comp.setInt("img_output", 0);
        Shader rect_vert{get_path("/shaders/rect.vert"), GL_VERTEX_SHADER};
        Shader rect_frag{get_path("/shaders/rect.frag"), GL_FRAGMENT_SHADER};
        Program rect{rect_vert, rect_frag};
        rect.setInt("frame", 0);

        while (glfwWindowShouldClose(window) == 0) {
            processInput(window);
            comp.use();
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
