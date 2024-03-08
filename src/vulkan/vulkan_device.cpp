#include <ranges>

#include "check.h"
#include "vulkan/vulkan_device.h"

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

vk::DynamicLoader load_vulkan() noexcept {
    vk::DynamicLoader ret{};
    auto vkGetInstanceProcAddr =
        ret.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    return ret;
}

vk::Instance create_instance(std::span<const char*> inst_ext,
    std::span<const char*> inst_layer) noexcept {
    vk::Result result;
    vk::Instance ret;
    vk::ApplicationInfo app_info{
        .pNext = nullptr,
        .pApplicationName = "RayTracing",
        .pEngineName = "NoEngine",
        .apiVersion = VK_API_VERSION_1_3,
    };
    {
        std::vector<vk::ExtensionProperties> provided_inst_ext{};
        VK_CHECK_CREATE(result, provided_inst_ext,
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
        VK_CHECK_CREATE(result, provided_inst_layer,
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
    VK_CHECK_CREATE(result, ret, vk::createInstance(instance_info));
    VULKAN_HPP_DEFAULT_DISPATCHER.init(ret);
    return ret;
}

vk::DebugUtilsMessengerEXT create_debug_messenger(vk::Instance inst) noexcept {
    vk::Result result;
    vk::DebugUtilsMessengerEXT ret;
    VK_CHECK_CREATE(result, ret,
        inst.createDebugUtilsMessengerEXT(vk::DebugUtilsMessengerCreateInfoEXT{
            .messageSeverity =
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            .pfnUserCallback = vulkan_debug_messenger_callback,
            .pUserData = nullptr,
        }));
    return ret;
}

vk::SurfaceKHR create_surface(
    vk::Instance inst, struct GLFWwindow* window) noexcept {
    VkSurfaceKHR surface_handle = VK_NULL_HANDLE;
    CHECK(glfwCreateWindowSurface(inst, window, nullptr, &surface_handle) ==
              VK_SUCCESS,
        "");
    vk::SurfaceKHR ret{surface_handle};
    return ret;
}

std::tuple<vk::Device, vk::PhysicalDevice, vulkan_queues>
    select_physical_device_create_device_queues(vk::Instance inst,
        vk::SurfaceKHR surface, std::span<const char*> dev_ext,
        const void* dev_creation_pnext) noexcept {
    vk::Result result;
    vk::Device ret_dev;
    vk::PhysicalDevice ret_phy_dev;
    vulkan_queues ret_queues{};
    std::vector<vk::PhysicalDevice> phy_devs{};
    VK_CHECK_CREATE(result, phy_devs, inst.enumeratePhysicalDevices());
    int32_t highest_phy_dev_score = -1;
    for (auto const& phy_dev : phy_devs) {
        std::optional<uint32_t> graphics_queue_idx{};
        std::optional<uint32_t> present_queue_idx{};
        std::optional<uint32_t> compute_queue_idx{};
        std::vector<vk::QueueFamilyProperties> queue_props{};
        queue_props = phy_dev.getQueueFamilyProperties();
        for (uint32_t i = 0; i < queue_props.size(); ++i) {
            vk::Bool32 support_present = false;
            VK_CHECK_CREATE(result, support_present,
                phy_dev.getSurfaceSupportKHR(i, surface));
            vk::Bool32 support_graphics =
                (queue_props[i].queueFlags & vk::QueueFlagBits::eGraphics) !=
                vk::QueueFlagBits{};
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
        if (!graphics_queue_idx.has_value() || !present_queue_idx.has_value() ||
            !compute_queue_idx.has_value()) {
            continue;
        }
        bool support_ext = true;
        {
            std::vector<vk::ExtensionProperties> provided_ext{};
            VK_CHECK_CREATE(result, provided_ext,
                phy_dev.enumerateDeviceExtensionProperties());
            for (auto const required_ext : dev_ext) {
                support_ext =
                    support_ext &&
                    std::ranges::any_of(provided_ext,
                        [required_ext](vk::ExtensionProperties const& prop) {
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
            ret_phy_dev = phy_dev;
            ret_queues.graphics_queue_idx = graphics_queue_idx.value();
            ret_queues.present_queue_idx = present_queue_idx.value();
            ret_queues.compute_queue_idx = compute_queue_idx.value();
        }
    }
    CHECK(ret_phy_dev, "");
    std::vector<uint32_t> queue_indices{ret_queues.graphics_queue_idx,
        ret_queues.present_queue_idx, ret_queues.compute_queue_idx};
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
    VK_CHECK_CREATE(result, ret_dev,
        ret_phy_dev.createDevice(vk::DeviceCreateInfo{
            .pNext = dev_creation_pnext,
            .queueCreateInfoCount = (uint32_t) queue_infos.size(),
            .pQueueCreateInfos = queue_infos.data(),
            .enabledExtensionCount = (uint32_t) dev_ext.size(),
            .ppEnabledExtensionNames = dev_ext.data(),
        }));
    VULKAN_HPP_DEFAULT_DISPATCHER.init(ret_dev);
    ret_queues.graphics_queue =
        ret_dev.getQueue(ret_queues.graphics_queue_idx, 0);
    ret_queues.present_queue =
        ret_dev.getQueue(ret_queues.present_queue_idx, 0);
    ret_queues.compute_queue =
        ret_dev.getQueue(ret_queues.compute_queue_idx, 0);
    return std::make_tuple(ret_dev, ret_phy_dev, ret_queues);
}

VmaAllocator create_vma_allocator(
    vk::Instance inst, vk::PhysicalDevice phy_dev, vk::Device dev) noexcept {
    VmaAllocator ret_alloc = VK_NULL_HANDLE;
    VmaVulkanFunctions vma_loading_func{
        .vkGetInstanceProcAddr =
            VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr =
            VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr,
    };
    VmaAllocatorCreateInfo alloc_info{
        .physicalDevice = phy_dev,
        .device = dev,
        .pVulkanFunctions = &vma_loading_func,
        .instance = inst,
        .vulkanApiVersion = VK_API_VERSION_1_3,
    };
    VkResult res = vmaCreateAllocator(&alloc_info, &ret_alloc);
    if (res != VK_SUCCESS) {
        fmt::println("Can't create vma allocator, returned {}",
            vk::to_string((vk::Result) res));
    }
    return ret_alloc;
}
