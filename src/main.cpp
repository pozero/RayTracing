#include <vector>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <cassert>
#include <span>
#include <array>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "fmt/core.h"
#include "GLFW/glfw3.h"
#pragma clang diagnostic pop

#include "VulkanDevice.h"

static void glfw_error_callback(int error, const char* desc) {
    fmt::println("GLFW Error ({}): {}", error, desc);
}

static GLFWwindow* create_window(int width, int height) noexcept {
    glfwInit();
    glfwSetErrorCallback(glfw_error_callback);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window =
        glfwCreateWindow(width, height, "RayTracing", nullptr, nullptr);
    return window;
}

int main() {
    int constexpr win_width = 800;
    int constexpr win_height = 600;
    GLFWwindow* window = create_window(win_width, win_height);
    VulkanDevice dev{};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
