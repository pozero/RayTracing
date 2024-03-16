#include "vulkan/vulkan_swapchain.h"
#include "vulkan/vulkan_render_pass.h"
#include "vulkan/vulkan_framebuffer.h"
#include "vulkan/vulkan_descriptor.h"
#include "vulkan/vulkan_pipeline.h"
#include "vulkan/vulkan_buffer.h"
#include "vulkan/vulkan_image.h"

#include "asset/camera.h"
#include "asset/scene.h"
#include "asset/texture.h"

#include "renderer/renderer.h"
#include "renderer/render_context.h"

#include "utils/to_span.h"

#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"

void rasterizer_initialize();
void rasterizer_prepare_data(scene const& scene);
void rasterizer_update_data(camera const& camera, scene const& scene);
void rasterizer_render();
void rasterizer_destroy();

void load_rasterizer(renderer& renderer) {
    renderer.initialize = rasterizer_initialize;
    renderer.prepare_data = rasterizer_prepare_data;
    renderer.update_data = rasterizer_update_data;
    renderer.render = rasterizer_render;
    renderer.destroy = rasterizer_destroy;
}

static void create_frame_objects();
static void destroy_frame_objects();
static void refresh_frame_objects();
static void create_disney_brdf_pipeline();
static void destroy_disney_brdf_pipeline();
static void prepare_disney_brdf_resources(scene const& scene);
static void clean_disney_brdf_resources();

static bool initialized = false;

static vk::DescriptorPool primary_descriptor_pool{};

static struct {
    vk::SwapchainKHR swapchain;
    std::vector<vk::ImageView> presents;
    vma_image depth;
    vma_image color;
    vk::RenderPass render_pass;
    std::vector<vk::Framebuffer> framebuffers;
} frame_objects{};

static void refresh_frame_objects() {
    wait_window(device, physical_device, surface, window);
    for (auto const framebuffer : frame_objects.framebuffers) {
        device.destroyFramebuffer(framebuffer);
    }
    for (auto const view : frame_objects.presents) {
        device.destroyImageView(view);
    }
    destroy_image(device, vma_alloc, frame_objects.depth);
    destroy_image(device, vma_alloc, frame_objects.color);
    device.destroySwapchainKHR(frame_objects.swapchain);
    std::tie(frame_objects.swapchain, frame_objects.presents,
        frame_objects.depth, frame_objects.color) =
        create_swapchain_with_depth_multisampling(
            device, vma_alloc, surface, command_queues);
    frame_objects.framebuffers = create_framebuffers_with_depth_multisampling(
        device, frame_objects.render_pass, frame_objects.presents,
        frame_objects.depth.primary_view, frame_objects.color.primary_view);
}

static void create_frame_objects() {
    std::tie(frame_objects.swapchain, frame_objects.presents,
        frame_objects.depth, frame_objects.color) =
        create_swapchain_with_depth_multisampling(
            device, vma_alloc, surface, command_queues);
    frame_objects.render_pass =
        create_render_pass_with_depth_multisampling(device);
    frame_objects.framebuffers = create_framebuffers_with_depth_multisampling(
        device, frame_objects.render_pass, frame_objects.presents,
        frame_objects.depth.primary_view, frame_objects.color.primary_view);
}

static void destroy_frame_objects() {
    for (auto const framebuffer : frame_objects.framebuffers) {
        device.destroyFramebuffer(framebuffer);
    }
    device.destroyRenderPass(frame_objects.render_pass);
    for (auto const view : frame_objects.presents) {
        device.destroyImageView(view);
    }
    destroy_image(device, vma_alloc, frame_objects.depth);
    destroy_image(device, vma_alloc, frame_objects.color);
    device.destroySwapchainKHR(frame_objects.swapchain);
}

static uint32_t constexpr DISNEY_BRDF_SET = 2;
static struct {
    // descriptors
    std::array<vk::DescriptorSetLayout, DISNEY_BRDF_SET> descriptor_layouts;
    std::array<std::array<vk::DescriptorSet, FRAME_IN_FLIGHT>, DISNEY_BRDF_SET>
        descriptor_sets;
    // pipeline
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;

    // resources
    vma_buffer indirect_draw_buffer;
    // set 0 resources
    vma_buffer vertices_buffer;
    vma_buffer transformation_buffer;
    vma_buffer normal_transformation_buffer;
    vma_buffer instance_material_index_buffer;
    // set 1 resources
    vma_buffer material_buffer;
    std::vector<vma_image> texture_array;
} disney_brdf{};

static void create_disney_brdf_pipeline() {
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
    uint32_t const texture_array_size =
        descriptor_indexing_properties
            .maxDescriptorSetUpdateAfterBindSampledImages;
    vk::DescriptorBindingFlags const texture_array_binding_flags =
        vk::DescriptorBindingFlagBits::eVariableDescriptorCount |
        vk::DescriptorBindingFlagBits::eUpdateAfterBind |
        vk::DescriptorBindingFlagBits::ePartiallyBound |
        vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
    vk_descriptor_set_binding constexpr single_storage_buffer_binding{
        vk::DescriptorType::eStorageBuffer, 1};
    vk_descriptor_set_binding const texture_array_binding{
        vk::DescriptorType::eCombinedImageSampler, texture_array_size,
        texture_array_binding_flags};
    std::array const bindings = {
        std::vector<vk_descriptor_set_binding>{
                                               single_storage_buffer_binding, single_storage_buffer_binding,
                                               single_storage_buffer_binding, single_storage_buffer_binding,
                                               },
        std::vector<vk_descriptor_set_binding>{
                                               single_storage_buffer_binding, texture_array_binding},
    };
    for (uint32_t i = 0; i < DISNEY_BRDF_SET; ++i) {
        disney_brdf.descriptor_layouts[i] = create_descriptor_set_layout(
            device, vk::ShaderStageFlagBits::eVertex, bindings[i]);
        create_descriptor_set(device, primary_descriptor_pool,
            disney_brdf.descriptor_layouts[i],
            to_span(disney_brdf.descriptor_sets[i]));
    }
    disney_brdf.pipeline_layout =
        create_pipeline_layout(device, to_span(disney_brdf.descriptor_layouts));
}

static void prepare_disney_brdf_resources(scene const& scene) {
    clean_disney_brdf_resources();
}

static void clean_disney_brdf_resources() {
    destroy_buffer(vma_alloc, disney_brdf.vertices_buffer);
    destroy_buffer(vma_alloc, disney_brdf.transformation_buffer);
    destroy_buffer(vma_alloc, disney_brdf.normal_transformation_buffer);
    destroy_buffer(vma_alloc, disney_brdf.instance_material_index_buffer);
    destroy_buffer(vma_alloc, disney_brdf.material_buffer);
    for (uint32_t i = 0; i < disney_brdf.texture_array.size(); ++i) {
        destroy_image(device, vma_alloc, disney_brdf.texture_array[i]);
    }
}

static void destroy_disney_brdf_pipeline() {
}

void rasterizer_initialize() {
    if (initialized) {
        return;
    }
    initialized = true;
}

void rasterizer_prepare_data(scene const& scene) {
}

void rasterizer_update_data(camera const& camera, scene const& scene) {
}

void rasterizer_render() {
}

void rasterizer_destroy() {
    if (!initialized) {
        return;
    }
    initialized = false;
    cleanup_staging_buffer(vma_alloc);
    cleanup_staging_image(vma_alloc);
}
