#include <bitset>

#include "check.h"
#include "window.h"
#include "vulkan/vulkan_device.h"
#include "vulkan/vulkan_swapchain.h"
#include "renderer/render_context.h"

static bool initialized = false;

// glfw
static uint32_t win_width = 1280;
static uint32_t win_height = 720;
GLFWwindow* window = nullptr;

// vulkan
static vk::Instance instance{};
#if defined(VK_DBG)
static vk::DebugUtilsMessengerEXT dbg_messenger{};
#endif
vk::SurfaceKHR surface{};
vk::Device device{};
vk::PhysicalDevice physical_device{};
vulkan_queues command_queues{};
VmaAllocator vma_alloc = nullptr;
static vk::CommandPool graphics_command_pool;
static vk::CommandPool compute_command_pool;
static uint32_t graphics_command_index = 0;
static uint32_t compute_command_index = 0;
static std::array<vk::CommandBuffer, FRAME_IN_FLIGHT> graphics_command_buffers;
static std::array<vk::CommandBuffer, FRAME_IN_FLIGHT> compute_command_buffers;
static std::array<vk::Fence, FRAME_IN_FLIGHT> graphics_command_fences;
static std::array<vk::Fence, FRAME_IN_FLIGHT> compute_command_fences;

void create_render_context() {
    if (initialized) {
        return;
    }
    /* GLFW */
    auto const glfw_resize_callback = [](GLFWwindow*, int width, int height) {
        win_width = (uint32_t) width;
        win_height = (uint32_t) height;
    };
    window = glfw_create_window((int) win_width, (int) win_height);
    glfwSetFramebufferSizeCallback(window, glfw_resize_callback);
    /* INSTANCE */
    std::vector<const char*> inst_ext{};
    std::vector<const char*> inst_layer{};
    std::vector<const char*> dev_ext{};
    inst_ext.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#if defined(VK_DBG)
    inst_ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    inst_layer.push_back("VK_LAYER_KHRONOS_validation");
#endif
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
        uint32_t glfw_required_inst_ext_cnt = 0;
        auto const glfw_required_inst_ext_name =
            glfwGetRequiredInstanceExtensions(&glfw_required_inst_ext_cnt);
        std::copy(glfw_required_inst_ext_name,
            glfw_required_inst_ext_name + glfw_required_inst_ext_cnt,
            std::back_inserter(inst_ext));
#pragma clang diagnostic pop
    }
    dev_ext.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    dev_ext.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    vk::DynamicLoader vk_loader = load_vulkan();
    instance = create_instance(inst_ext, inst_layer);
    /* DEBUG MESSENGER */
#if defined(VK_DBG)
    dbg_messenger = create_debug_messenger(instance);
#endif
    /* SURFACE */
    surface = create_surface(instance, window);
    /* DEVICE */
    const void* dev_creation_pnext = nullptr;
    vk::PhysicalDeviceDescriptorIndexingFeatures const
        descriptor_indexing_features{
            .shaderSampledImageArrayNonUniformIndexing = vk::True,
            .descriptorBindingSampledImageUpdateAfterBind = vk::True,
            .descriptorBindingUpdateUnusedWhilePending = vk::True,
            .descriptorBindingPartiallyBound = vk::True,
            .descriptorBindingVariableDescriptorCount = vk::True,
            .runtimeDescriptorArray = vk::True,
        };
    vk::PhysicalDeviceSynchronization2Features const synchron2_feature{
        .pNext = (void*) &descriptor_indexing_features,
        .synchronization2 = VK_TRUE,
    };
    dev_creation_pnext = &synchron2_feature;
    vk::PhysicalDeviceFeatures const physical_device_deature{
        .sampleRateShading = vk::True,
        .fillModeNonSolid = vk::True,
    };
    std::tie(device, physical_device, command_queues) =
        select_physical_device_create_device_queues(instance, surface, dev_ext,
            dev_creation_pnext, physical_device_deature);
    VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptor_indexing_properties{
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT,
    };
    VkPhysicalDeviceProperties2KHR phy_dev_properties{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR,
        .pNext = &descriptor_indexing_properties,
    };
    vk::defaultDispatchLoaderDynamic.vkGetPhysicalDeviceProperties2(
        physical_device, &phy_dev_properties);
    VkSampleCountFlags const multisample_count_flag =
        phy_dev_properties.properties.limits.framebufferColorSampleCounts &
        phy_dev_properties.properties.limits.framebufferDepthSampleCounts;
    if (multisample_count_flag & VK_SAMPLE_COUNT_64_BIT) {
        multisample_count = vk::SampleCountFlagBits::e64;
    } else if (multisample_count_flag & VK_SAMPLE_COUNT_32_BIT) {
        multisample_count = vk::SampleCountFlagBits::e32;
    } else if (multisample_count_flag & VK_SAMPLE_COUNT_16_BIT) {
        multisample_count = vk::SampleCountFlagBits::e16;
    } else if (multisample_count_flag & VK_SAMPLE_COUNT_8_BIT) {
        multisample_count = vk::SampleCountFlagBits::e8;
    } else if (multisample_count_flag & VK_SAMPLE_COUNT_4_BIT) {
        multisample_count = vk::SampleCountFlagBits::e4;
    } else if (multisample_count_flag & VK_SAMPLE_COUNT_2_BIT) {
        multisample_count = vk::SampleCountFlagBits::e2;
    }
    fmt::println("Selected device: {}",
        physical_device.getProperties().deviceName.data());
    /* VMA */
    vma_alloc = create_vma_allocator(instance, physical_device, device);
    /* COMMAND BUFFERS */
    vk::Result result;
    vk::CommandPoolCreateInfo command_pool_info{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = command_queues.graphics_queue_idx,
    };
    VK_CHECK_CREATE(result, graphics_command_pool,
        device.createCommandPool(command_pool_info));
    command_pool_info.queueFamilyIndex = command_queues.compute_queue_idx;
    VK_CHECK_CREATE(result, compute_command_pool,
        device.createCommandPool(command_pool_info));
    vk::CommandBufferAllocateInfo command_buffer_allocation_info{
        .commandPool = graphics_command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = FRAME_IN_FLIGHT,
    };
    VK_CHECK(
        result, device.allocateCommandBuffers(&command_buffer_allocation_info,
                    graphics_command_buffers.data()));
    command_buffer_allocation_info.commandPool = compute_command_pool;
    VK_CHECK(
        result, device.allocateCommandBuffers(&command_buffer_allocation_info,
                    compute_command_buffers.data()));
    vk::FenceCreateInfo const fence_info{
        .flags = vk::FenceCreateFlagBits::eSignaled,
    };
    for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {
        VK_CHECK_CREATE(
            result, graphics_command_fences[i], device.createFence(fence_info));
        VK_CHECK_CREATE(
            result, compute_command_fences[i], device.createFence(fence_info));
    }
    initialized = true;
}

void destroy_render_context() {
    if (!initialized) {
        return;
    }
    for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {
        device.destroyFence(graphics_command_fences[i]);
        device.destroyFence(compute_command_fences[i]);
    }
    device.destroyCommandPool(graphics_command_pool);
    device.destroyCommandPool(compute_command_pool);
    vmaDestroyAllocator(vma_alloc);
    device.destroy();
    instance.destroySurfaceKHR(surface);
#if defined(VK_DBG)
    instance.destroyDebugUtilsMessengerEXT(dbg_messenger);
#endif
    instance.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
    initialized = false;
}

vk::CommandBuffer get_command_buffer(vk::PipelineBindPoint bind_point) {
    vk::Result result;
    vk::CommandBuffer command_buffer;
    vk::Fence command_fence;
    if (bind_point == vk::PipelineBindPoint::eGraphics) {
        command_buffer = graphics_command_buffers[graphics_command_index];
        command_fence = graphics_command_fences[graphics_command_index];
    } else {
        command_buffer = compute_command_buffers[compute_command_index];
        command_fence = compute_command_fences[compute_command_index];
    }
    VK_CHECK(result, device.waitForFences(1, &command_fence, true, 1e9));
    return command_buffer;
}

void submit_command_buffer(vk::PipelineBindPoint bind_point,
    std::vector<vk::Semaphore> const& wait_semphores,
    std::vector<vk::PipelineStageFlags> const& wait_stages,
    std::vector<vk::Semaphore> const& signal_semphores) {
    vk::Result result;
    vk::CommandBuffer command_buffer;
    vk::Fence command_fence;
    vk::Queue command_queue;
    if (bind_point == vk::PipelineBindPoint::eGraphics) {
        command_buffer = graphics_command_buffers[graphics_command_index];
        command_fence = graphics_command_fences[graphics_command_index];
        command_queue = command_queues.graphics_queue;
        graphics_command_index = (graphics_command_index + 1) % FRAME_IN_FLIGHT;
    } else {
        command_buffer = compute_command_buffers[compute_command_index];
        command_fence = compute_command_fences[compute_command_index];
        command_queue = command_queues.compute_queue;
        compute_command_index = (compute_command_index + 1) % FRAME_IN_FLIGHT;
    }
    vk::SubmitInfo const submit_info{
        .waitSemaphoreCount = (uint32_t) wait_semphores.size(),
        .pWaitSemaphores = wait_semphores.data(),
        .pWaitDstStageMask = wait_stages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = (uint32_t) signal_semphores.size(),
        .pSignalSemaphores = signal_semphores.data(),
    };
    VK_CHECK(result, command_queue.submit(1, &submit_info, command_fence));
}
