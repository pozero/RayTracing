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

#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
int main() {
    int constexpr win_width = 800;
    int constexpr win_height = 600;
    GLFWwindow* window = create_window(win_width, win_height);
    std::vector<const char*> inst_ext{};
    std::vector<const char*> inst_layer{};
    std::vector<const char*> dev_ext{};
    {
        uint32_t glfw_required_inst_ext_cnt = 0;
        auto const glfw_required_inst_ext_name =
            glfwGetRequiredInstanceExtensions(&glfw_required_inst_ext_cnt);
        std::copy(glfw_required_inst_ext_name,
            glfw_required_inst_ext_name + glfw_required_inst_ext_cnt,
            std::back_inserter(inst_ext));
    }
    dev_ext.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#if defined(VK_DBG)
    inst_ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    inst_layer.push_back("VK_LAYER_KHRONOS_validation");
#endif
    const void* dev_creation_pnext = nullptr;
    vk::PhysicalDeviceSynchronization2Features const synchron2_feature{
        .synchronization2 = VK_TRUE};
    dev_creation_pnext = &synchron2_feature;
    vk::DynamicLoader vk_loader = load_vulkan();
    vk::Instance vk_inst = create_instance(inst_ext, inst_layer);
    [[maybe_unused]] vk::DebugUtilsMessengerEXT vk_dbg_messenger =
        create_debug_messenger(vk_inst);
    vk::SurfaceKHR vk_surface = create_surface(vk_inst, window);
    auto [vk_dev, vk_phy_dev, queue_infos] =
        select_physical_device_create_device_queues(
            vk_inst, vk_surface, dev_ext, dev_creation_pnext);
    VmaAllocator vma_alloc = create_vma_allocator(vk_inst, vk_phy_dev, vk_dev);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    glfwTerminate();
    vmaDestroyAllocator(vma_alloc);
    vk_dev.destroy();
    vk_inst.destroySurfaceKHR(vk_surface);
    vk_inst.destroyDebugUtilsMessengerEXT(vk_dbg_messenger);
    vk_inst.destroy();
    return 0;
}
