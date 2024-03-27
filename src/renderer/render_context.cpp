#include <bitset>

#include "check.h"
#include "window.h"
#include "vulkan/vulkan_device.h"
#include "vulkan/vulkan_swapchain.h"
#include "vulkan/vulkan_buffer.h"
#include "renderer/render_context.h"

#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"

static bool initialized = false;

// glfw
uint32_t win_width = 1280;
uint32_t win_height = 720;
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
static struct vk_commands {
    vk::CommandPool pool;
    uint32_t index = 0;
    bool new_buffer = true;
    std::array<vk::CommandBuffer, FRAME_IN_FLIGHT> buffers;
    std::array<vk::Fence, FRAME_IN_FLIGHT> fences;
    std::vector<vk::Semaphore> wait_semphores;
    std::vector<vk::PipelineStageFlags> wait_stages;
    std::vector<vk::Semaphore> signal_semphores;
} graphics_commands{}, compute_commands{};
static std::vector<vk::Semaphore> present_semaphore{};

bool window_should_close() {
    return glfwWindowShouldClose(window);
}

void poll_window_event() {
    glfwPollEvents();
}

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
        .synchronization2 = vk::True,
    };
    dev_creation_pnext = &synchron2_feature;
    vk::PhysicalDeviceFeatures const physical_device_deature{
        .sampleRateShading = vk::True,
        .multiDrawIndirect = vk::True,
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
    VK_CHECK_CREATE(result, graphics_commands.pool,
        device.createCommandPool(command_pool_info));
    command_pool_info.queueFamilyIndex = command_queues.compute_queue_idx;
    VK_CHECK_CREATE(result, compute_commands.pool,
        device.createCommandPool(command_pool_info));
    vk::CommandBufferAllocateInfo command_buffer_allocation_info{
        .commandPool = graphics_commands.pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = FRAME_IN_FLIGHT,
    };
    VK_CHECK(
        result, device.allocateCommandBuffers(&command_buffer_allocation_info,
                    graphics_commands.buffers.data()));
    command_buffer_allocation_info.commandPool = compute_commands.pool;
    VK_CHECK(
        result, device.allocateCommandBuffers(&command_buffer_allocation_info,
                    compute_commands.buffers.data()));
    vk::FenceCreateInfo const fence_info{
        .flags = vk::FenceCreateFlagBits::eSignaled,
    };
    for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {
        VK_CHECK_CREATE(result, graphics_commands.fences[i],
            device.createFence(fence_info));
        VK_CHECK_CREATE(
            result, compute_commands.fences[i], device.createFence(fence_info));
    }
    /* SWAPCHAIN PREPARE */
    prepare_swapchain(physical_device, surface);
    wait_window(device, physical_device, surface, window);
    initialized = true;
    /* CREATE DUMMY BUFFER */
    create_dummy_buffer(vma_alloc);
}

void destroy_render_context() {
    if (!initialized) {
        return;
    }
    destroy_dummy_buffer(vma_alloc);
    for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {
        device.destroyFence(graphics_commands.fences[i]);
        device.destroyFence(compute_commands.fences[i]);
    }
    device.destroyCommandPool(graphics_commands.pool);
    device.destroyCommandPool(compute_commands.pool);
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

void wait_vulkan() {
    vk::Result result;
    VK_CHECK(result, device.waitIdle());
}

std::pair<vk::CommandBuffer, uint32_t> get_command_buffer(
    vk::PipelineBindPoint bind_point) {
    vk::Result result;
    vk_commands& commands = bind_point == vk::PipelineBindPoint::eGraphics ?
                                graphics_commands :
                                compute_commands;
    vk::CommandBuffer const command_buffer = commands.buffers[commands.index];
    vk::Fence const command_fence = commands.fences[commands.index];
    vk::CommandBufferBeginInfo const begin_info{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    if (commands.new_buffer) {
        commands.new_buffer = false;
        VK_CHECK(result, device.waitForFences(1, &command_fence, true, 1e9));
        VK_CHECK(result, device.resetFences(1, &command_fence));
        VK_CHECK(result, command_buffer.reset());
        VK_CHECK(result, command_buffer.begin(begin_info));
    }
    return std::make_pair(command_buffer, commands.index);
}

void add_submit_wait(vk::PipelineBindPoint bind_point, vk::Semaphore semaphore,
    vk::PipelineStageFlags stage) {
    vk_commands& commands = bind_point == vk::PipelineBindPoint::eGraphics ?
                                graphics_commands :
                                compute_commands;
    commands.wait_semphores.push_back(semaphore);
    commands.wait_stages.push_back(stage);
}

void add_submit_signal(
    vk::PipelineBindPoint bind_point, vk::Semaphore semaphore) {
    vk_commands& commands = bind_point == vk::PipelineBindPoint::eGraphics ?
                                graphics_commands :
                                compute_commands;
    commands.signal_semphores.push_back(semaphore);
}

void submit_command_buffer(vk::PipelineBindPoint bind_point) {
    vk::Result result;
    bool const graphics = bind_point == vk::PipelineBindPoint::eGraphics;
    vk_commands& commands = graphics ? graphics_commands : compute_commands;
    vk::Queue const command_queue =
        graphics ? command_queues.graphics_queue : command_queues.compute_queue;
    vk::CommandBuffer const command_buffer = commands.buffers[commands.index];
    vk::Fence const command_fence = commands.fences[commands.index];
    commands.index = (commands.index + 1) % FRAME_IN_FLIGHT;
    commands.new_buffer = true;
    VK_CHECK(result, command_buffer.end());
    vk::SubmitInfo const submit_info{
        .waitSemaphoreCount = (uint32_t) commands.wait_semphores.size(),
        .pWaitSemaphores = commands.wait_semphores.data(),
        .pWaitDstStageMask = commands.wait_stages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = (uint32_t) commands.signal_semphores.size(),
        .pSignalSemaphores = commands.signal_semphores.data(),
    };
    VK_CHECK(result, command_queue.submit(1, &submit_info, command_fence));
    commands.wait_semphores.clear();
    commands.wait_stages.clear();
    commands.signal_semphores.clear();
}

void add_present_wait(vk::Semaphore semaphore) {
    present_semaphore.push_back(semaphore);
}

vk::Result present(
    vk::SwapchainKHR const& swapchain, uint32_t const& image_idx) {
    vk::PresentInfoKHR const present_info{
        .waitSemaphoreCount = (uint32_t) present_semaphore.size(),
        .pWaitSemaphores = present_semaphore.data(),
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &image_idx,
    };
    vk::Result const result =
        swapchain_present_wrapper(command_queues.present_queue, present_info);
    present_semaphore.clear();
    return result;
}
