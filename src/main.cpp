#include <iostream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "fmt/core.h"
#include "fmt/color.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#pragma clang diagnostic pop

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window =
        glfwCreateWindow(800, 600, "RayTracing", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    int gl_ver = gladLoadGLLoader(
        [](const char* s) -> void* { return (void*) glfwGetProcAddress(s); });
    fmt::println("GL {}", gl_ver);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}
