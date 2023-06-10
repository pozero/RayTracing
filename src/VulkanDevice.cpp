#include "VulkanDevice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "fmt/core.h"
#pragma clang diagnostic pop

VulkanDevice::VulkanDevice() noexcept {
    vk::Result result;
    vk::DynamicLoader dyna_loader;
    auto vkGetInstanceProcAddr =
        dyna_loader.getProcAddress<PFN_vkGetInstanceProcAddr>(
            "vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    vk::ApplicationInfo app_info{
        .pNext = nullptr,
        .pApplicationName = "RayTracing",
        .pEngineName = "NoEngine",
        .apiVersion = VK_API_VERSION_1_3,
    };
    vk::InstanceCreateInfo instance_info{
        .pApplicationInfo = &app_info,
    };
    std::tie(result, m_instance) = vk::createInstance(instance_info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);
    fmt::println("{}", vk::to_string(result));
}

VulkanDevice::~VulkanDevice() noexcept {
    vk::DynamicLoader dyna_loader;
    auto vkDestroyInstance =
        dyna_loader.getProcAddress<PFN_vkDestroyInstance>("vkDestroyInstance");
    fmt::println("{}",
        vkDestroyInstance == VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyInstance);
    VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyInstance = vkDestroyInstance;
    m_instance.destroy();
}
