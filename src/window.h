#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "fmt/core.h"
#include "GLFW/glfw3.h"
#pragma clang diagnostic pop

inline void glfw_error_callback(int error, const char* desc) {
    fmt::println("GLFW Error ({}): {}", error, desc);
}

inline GLFWwindow* glfw_create_window(int width, int height) {
    glfwInit();
    glfwSetErrorCallback(glfw_error_callback);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    GLFWwindow* window =
        glfwCreateWindow(width, height, "RayTracing", nullptr, nullptr);
    return window;
}
