#include <ranges>

#include "VulkanDevice.h"
#include "Check.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "GLFW/glfw3.h"
#pragma clang diagnostic pop

constexpr std::string_view to_string_view(
    VkDebugUtilsMessageSeverityFlagBitsEXT messenger_severity) noexcept {
    switch (messenger_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return "verbose";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            return "info";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            return "warning";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            return "error";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
            return "unknown";
    }
}

#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
inline VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    [[maybe_unused]] void* user_data) {
    std::string message{callback_data->pMessage};
    for (uint32_t i = 0; i < callback_data->objectCount; ++i) {
        if (callback_data->pObjects[i].pObjectName) {
            message += "\n\t";
            message += callback_data->pObjects[i].pObjectName;
        }
    }
    fmt::println("Vulkan callback: [{}] {}", to_string_view(severity), message);
    return VK_FALSE;
}

VulkanDevice::VulkanDevice(std::span<const char*> inst_ext,
    std::span<const char*> inst_layer, struct GLFWwindow* window,
    std::span<const char*> dev_ext, const void* dev_creation_pnext) noexcept {
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
    {
        std::vector<vk::ExtensionProperties> provided_inst_ext{};
        VK_CHECK(result, provided_inst_ext,
            vk::enumerateInstanceExtensionProperties());
        for (auto const required_ext : inst_ext) {
            bool provided = std::ranges::any_of(provided_inst_ext,
                [required_ext](vk::ExtensionProperties const& prop) {
                    return strcmp(prop.extensionName.data(), required_ext);
                });
            CHECK(provided, "Can't find instance extension {}", required_ext);
        }
    }
    {
        std::vector<vk::LayerProperties> provided_inst_layer{};
        VK_CHECK(result, provided_inst_layer,
            vk::enumerateInstanceLayerProperties());
        for (auto const required_layer : inst_layer) {
            bool provided = std::ranges::any_of(provided_inst_layer,
                [required_layer](vk::LayerProperties const& prop) {
                    return strcmp(prop.layerName.data(), required_layer);
                });
            CHECK(provided, "Can't find instance layer {}", required_layer);
        }
    }
    vk::InstanceCreateInfo instance_info{
        .pApplicationInfo = &app_info,
        .enabledLayerCount = (uint32_t) inst_layer.size(),
        .ppEnabledLayerNames = inst_layer.data(),
        .enabledExtensionCount = (uint32_t) inst_ext.size(),
        .ppEnabledExtensionNames = inst_ext.data(),
    };
    VK_CHECK(result, m_instance, vk::createInstance(instance_info));
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);
#if defined(VK_DBG)
    VK_CHECK(result, m_dbg_messenger,
        m_instance.createDebugUtilsMessengerEXT(
            vk::DebugUtilsMessengerCreateInfoEXT{
                .messageSeverity =
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                .messageType =
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                .pfnUserCallback = vulkan_debug_messenger_callback,
                .pUserData = nullptr,
            }));
#endif
    VkSurfaceKHR surface_handle = VK_NULL_HANDLE;
    CHECK(glfwCreateWindowSurface(
              m_instance, window, nullptr, &surface_handle) == VK_SUCCESS,
        "");
    m_surface = surface_handle;
    {
        std::vector<vk::PhysicalDevice> phy_devs{};
        VK_CHECK(result, phy_devs, m_instance.enumeratePhysicalDevices());
        int32_t highest_phy_dev_score = -1;
        for (auto const& phy_dev : phy_devs) {
            std::optional<uint32_t> graphics_queue_idx{};
            std::optional<uint32_t> present_queue_idx{};
            std::optional<uint32_t> compute_queue_idx{};
            std::vector<vk::QueueFamilyProperties> queue_props{};
            queue_props = phy_dev.getQueueFamilyProperties();
            for (uint32_t i = 0; i < queue_props.size(); ++i) {
                vk::Bool32 support_present = false;
                VK_CHECK(result, support_present,
                    phy_dev.getSurfaceSupportKHR(i, m_surface));
                vk::Bool32 support_graphics =
                    (queue_props[i].queueFlags &
                        vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits{};
                if (support_present && support_graphics) {
                    graphics_queue_idx = i;
                    present_queue_idx = i;
                    break;
                }
                if (!graphics_queue_idx.has_value() && support_graphics) {
                    graphics_queue_idx = i;
                }
                if (!present_queue_idx.has_value() && support_present) {
                    present_queue_idx = i;
                }
            }
            for (uint32_t i = 0; i < queue_props.size(); ++i) {
                bool const support_compute =
                    (queue_props[i].queueFlags & vk::QueueFlagBits::eCompute) !=
                    vk::QueueFlagBits{};
                if (!support_compute)
                    continue;
                if (graphics_queue_idx.has_value() &&
                    graphics_queue_idx.value() != i) {
                    compute_queue_idx = i;
                    break;
                }
                if (!compute_queue_idx.has_value()) {
                    compute_queue_idx = i;
                }
            }
            if (!graphics_queue_idx.has_value() ||
                !present_queue_idx.has_value() ||
                !compute_queue_idx.has_value()) {
                continue;
            }
            bool support_ext = true;
            {
                std::vector<vk::ExtensionProperties> provided_ext{};
                VK_CHECK(result, provided_ext,
                    phy_dev.enumerateDeviceExtensionProperties());
                for (auto const required_ext : dev_ext) {
                    support_ext =
                        support_ext &&
                        std::ranges::any_of(provided_ext,
                            [required_ext](
                                vk::ExtensionProperties const& prop) {
                                return strcmp(
                                    prop.extensionName.data(), required_ext);
                            });
                }
            }
            if (!support_ext) {
                continue;
            }
            auto property = phy_dev.getProperties();
            int score = 0;
            if (property.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                score += 1000;
            } else if (property.deviceType ==
                       vk::PhysicalDeviceType::eIntegratedGpu) {
                score += 100;
            }
            if (graphics_queue_idx.value() == present_queue_idx.value()) {
                score += 50;
            }
            if (graphics_queue_idx.value() != compute_queue_idx.value()) {
                score += 50;
            }
            if (score > highest_phy_dev_score) {
                highest_phy_dev_score = score;
                m_physical_device = phy_dev;
                m_graphics_queue_idx = graphics_queue_idx.value();
                m_present_queue_idx = present_queue_idx.value();
                m_compute_queue_idx = compute_queue_idx.value();
            }
        }
        CHECK(m_physical_device, "");
    }
    {
        std::vector<uint32_t> queue_indices{
            m_graphics_queue_idx, m_present_queue_idx, m_compute_queue_idx};
        auto [erase_begin, erase_end] = std::ranges::unique(queue_indices);
        queue_indices.erase(erase_begin, erase_end);
        std::vector<vk::DeviceQueueCreateInfo> queue_infos{};
        queue_infos.reserve(queue_indices.size());
        float constexpr queue_priority = 1.0f;
        for (uint32_t const idx : queue_indices) {
            queue_infos.push_back(vk::DeviceQueueCreateInfo{
                .queueFamilyIndex = idx,
                .queueCount = 1,
                .pQueuePriorities = &queue_priority,
            });
        }
        VK_CHECK(result, m_device,
            m_physical_device.createDevice(vk::DeviceCreateInfo{
                .pNext = dev_creation_pnext,
                .queueCreateInfoCount = (uint32_t) queue_infos.size(),
                .pQueueCreateInfos = queue_infos.data(),
                .enabledExtensionCount = (uint32_t) dev_ext.size(),
                .ppEnabledExtensionNames = dev_ext.data(),
            }));
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);
        m_graphics_queue = m_device.getQueue(m_graphics_queue_idx, 0);
        m_present_queue = m_device.getQueue(m_present_queue_idx, 0);
        m_compute_queue = m_device.getQueue(m_compute_queue_idx, 0);
    }
    {
        VmaVulkanFunctions vma_loading_func{
            .vkGetInstanceProcAddr =
                VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr =
                VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr,
        };
        VmaAllocatorCreateInfo alloc_info{
            .physicalDevice = m_physical_device,
            .device = m_device,
            .pVulkanFunctions = &vma_loading_func,
            .instance = m_instance,
            .vulkanApiVersion = VK_API_VERSION_1_3,
        };
        VkResult res = vmaCreateAllocator(&alloc_info, &m_vma_alloc);
        if (res != VK_SUCCESS) {
            fmt::println("Can't create vma allocator, returned {}", (int) res);
        }
    }
}

VulkanDevice::~VulkanDevice() noexcept {
    vk::DynamicLoader dyna_loader;
    auto vkGetInstanceProcAddr =
        dyna_loader.getProcAddress<PFN_vkGetInstanceProcAddr>(
            "vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);
    // VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);
    vmaDestroyAllocator(m_vma_alloc);
    m_device.destroy();
    m_instance.destroySurfaceKHR(m_surface);
    m_instance.destroyDebugUtilsMessengerEXT(m_dbg_messenger);
    m_instance.destroy();
}
