#include <random>

#include "check.h"
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
#include "renderer/bvh.h"

#include "utils/to_span.h"
#include "utils/file.h"

#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"

void megakernel_raytracer_initialize(render_options const& options);
void megakernel_raytracer_prepare_data(scene const& scene);
void megakernel_raytracer_update_data(scene const& scene);
void megakernel_raytracer_render(camera const& camera);
void megakernel_raytracer_present();
void megakernel_raytracer_destroy();

void load_megakernel_raytracer(renderer& renderer);

static void create_frame_objects();
static void destroy_frame_objects();
static void refresh_frame_objects();
static void create_megakernel_raytracer_pipeline();
static void destroy_megakernel_raytracer_pipeline();
static void prepare_megakernel_raytracer_resources(scene const& scene);
static void clean_megakernel_raytracer_resources();
static void create_rect_pipeline();
static void destroy_rect_pipeline();
static void prepare_rect_resources();

static bool initialized = false;

static struct {
    glm::uvec2 count{0};
    glm::uvec2 size{0};
    glm::uvec2 current{0};
} tiles;

uint32_t constexpr PREVIEW_RATIO = 5;
static uint32_t preview_width;
static uint32_t preview_height;

static bool next_tile() {
    if (tiles.current.x != tiles.count.x - 1) {
        ++tiles.current.x;
        return false;
    } else {
        tiles.current.x = 0;
        if (tiles.current.y != tiles.count.y - 1) {
            ++tiles.current.y;
            return false;
        } else {
            tiles.current.y = 0;
            return true;
        }
    }
}

static glm::uvec2 current_viewport() {
    uint32_t const offset_x = tiles.current.x * tiles.size.x;
    uint32_t const offset_y = tiles.current.y * tiles.size.y;
    return glm::uvec2{offset_x, offset_y};
}

static uint32_t rand_uint() {
    static std::random_device rand_dev{};
    static std::mt19937 rand_gen{rand_dev()};
    static std::uniform_int_distribution<uint32_t> uni_dist{
        0, std::numeric_limits<uint32_t>::max()};
    return uni_dist(rand_gen);
}

static vk::DescriptorPool primary_descriptor_pool{};
static vk::DescriptorPool indexing_descriptor_pool{};
static vk::Sampler primary_sampler{};
static vk::Sampler blocky_sampler{};
static std::array<vk::Semaphore, FRAME_IN_FLIGHT> compute_semaphores{};
static std::array<vk::Semaphore, FRAME_IN_FLIGHT> graphics_semaphores{};
static std::array<vk::Semaphore, FRAME_IN_FLIGHT> present_semaphores{};

static struct {
    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> swapchain_image;
    std::vector<vk::ImageView> presents;
    vk::RenderPass render_pass;
    std::vector<vk::Framebuffer> framebuffers;
    uint32_t swapchain_image_idx;
} frame_objects{};

static uint32_t constexpr MEGAKERNAL_RAYTRACER_SET = 4;

static uint32_t accumulation_counter = 0;

static struct {
    // descriptors
    std::array<vk::DescriptorSetLayout, MEGAKERNAL_RAYTRACER_SET>
        descriptor_layouts;
    std::array<std::array<vk::DescriptorSet, FRAME_IN_FLIGHT>,
        MEGAKERNAL_RAYTRACER_SET>
        descriptor_sets;
    // pipeline
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;
    // resources
    // set 0
    vk_buffer tlas_buffer;
    vk_buffer blas_buffer;
    // set 1
    vk_buffer mesh_buffer;
    vk_buffer transform_buffer;
    vk_buffer inverse_transform_buffer;
    vk_buffer instance_buffer;
    vk_buffer triangle_buffer;
    // set 2
    vk_buffer material_buffer;
    vk_buffer medium_buffer;
    vk_buffer light_buffer;
    // set 3
    vk_image accumulation_image;  // rendered by tiles for accumulation
    vk_image preview_image;       // low resolution for preview
    std::vector<vk_image> texture_array;
    // others
    vk_image output_image;  // color from scratch image would be copied to
                            // this image after all tiles get rendered
} megakernel_raytracer;

static uint32_t max_tracing_depth = 0;
static uint32_t light_count = 0;
static int32_t sky_light_idx = -1;

struct megakernel_raytracer_pc {
    glsl_raytracer_camera camera;
    uint32_t random_seed;
    uint32_t preview;
    uint32_t max_depth;
    uint32_t light_count;
    int32_t sky_light;
};

static struct {
    // descriptors
    vk::DescriptorSetLayout descriptor_layout;
    std::array<vk::DescriptorSet, FRAME_IN_FLIGHT> descriptor_sets;
    // pipeline
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;
} rect;

struct rect_pc {
    float frame_scalar;
    uint32_t preview;
};

static void create_frame_objects() {
    std::tie(frame_objects.swapchain, frame_objects.swapchain_image,
        frame_objects.presents) =
        create_swapchain(device, surface, command_queues);
    frame_objects.render_pass = create_render_pass(device);
    frame_objects.framebuffers = create_framebuffers(
        device, frame_objects.render_pass, frame_objects.presents);
}

static void destroy_frame_objects() {
    for (auto const framebuffer : frame_objects.framebuffers) {
        device.destroyFramebuffer(framebuffer);
    }
    device.destroyRenderPass(frame_objects.render_pass);
    for (auto const view : frame_objects.presents) {
        device.destroyImageView(view);
    }
    device.destroySwapchainKHR(frame_objects.swapchain);
}

static void refresh_frame_objects() {
    wait_window(device, physical_device, surface, window);
    for (auto const framebuffer : frame_objects.framebuffers) {
        device.destroyFramebuffer(framebuffer);
    }
    for (auto const view : frame_objects.presents) {
        device.destroyImageView(view);
    }
    device.destroySwapchainKHR(frame_objects.swapchain);
    std::tie(frame_objects.swapchain, frame_objects.swapchain_image,
        frame_objects.presents) =
        create_swapchain(device, surface, command_queues);
    frame_objects.framebuffers = create_framebuffers(
        device, frame_objects.render_pass, frame_objects.presents);
}

static void create_megakernel_raytracer_pipeline() {
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
    std::array const bindings{
        std::vector<vk_descriptor_set_binding>{
                                               {vk::DescriptorType::eStorageBuffer, 1},
                                               {vk::DescriptorType::eStorageBuffer, 1}},
        std::vector<vk_descriptor_set_binding>{
                                               {vk::DescriptorType::eStorageBuffer, 1},
                                               {vk::DescriptorType::eStorageBuffer, 1},
                                               {vk::DescriptorType::eStorageBuffer, 1},
                                               {vk::DescriptorType::eStorageBuffer, 1},
                                               {vk::DescriptorType::eStorageBuffer, 1}},
        std::vector<vk_descriptor_set_binding>{
                                               {vk::DescriptorType::eStorageBuffer, 1},
                                               {vk::DescriptorType::eStorageBuffer, 1},
                                               {vk::DescriptorType::eStorageBuffer, 1}},
        std::vector<vk_descriptor_set_binding>{
                                               {vk::DescriptorType::eStorageImage, 2},
                                               {vk::DescriptorType::eCombinedImageSampler, texture_array_size,
                texture_array_binding_flags}},
    };
    for (uint32_t s = 0; s < MEGAKERNAL_RAYTRACER_SET; ++s) {
        megakernel_raytracer
            .descriptor_layouts[s] = create_descriptor_set_layout(device,
            vk::ShaderStageFlagBits::eCompute, bindings[s],
            s == 3 ?
                vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool :
                vk::DescriptorSetLayoutCreateFlagBits{0});
        create_descriptor_set(device,
            s == 3 ? indexing_descriptor_pool : primary_descriptor_pool,
            megakernel_raytracer.descriptor_layouts[s],
            megakernel_raytracer.descriptor_sets[s]);
    }
    std::array pc_sizes{(uint32_t) sizeof(megakernel_raytracer_pc)};
    std::array pc_stages{vk::ShaderStageFlagBits::eCompute};
    megakernel_raytracer.pipeline_layout = create_pipeline_layout(
        device, pc_sizes, pc_stages, megakernel_raytracer.descriptor_layouts);
    megakernel_raytracer.pipeline = create_compute_pipeline(device,
        PATH_FROM_BINARY("shaders/megakernel_raytracer.comp.spv"),
        megakernel_raytracer.pipeline_layout, {},
        vk::PipelineCreateFlagBits::eDispatchBase);
}

static void destroy_megakernel_raytracer_pipeline() {
    for (uint32_t s = 0; s < MEGAKERNAL_RAYTRACER_SET; ++s) {
        device.destroyDescriptorSetLayout(
            megakernel_raytracer.descriptor_layouts[s]);
    }
    device.destroyPipelineLayout(megakernel_raytracer.pipeline_layout);
    device.destroyPipeline(megakernel_raytracer.pipeline);
}

static void prepare_megakernel_raytracer_resources(scene const& scene) {
    clean_megakernel_raytracer_resources();
    light_count = (uint32_t) scene.lights.size();
    sky_light_idx = scene.lights.back().type == light_type::sky ?
                        (int32_t) scene.lights.size() - 1 :
                        -1;
    bvh const bvh = create_bvh(scene);
    std::vector<glm::mat4> inverse_transformations{};
    inverse_transformations.reserve(scene.transformation.size());
    for (uint32_t t = 0; t < scene.transformation.size(); ++t) {
        inverse_transformations.push_back(
            glm::inverse(scene.transformation[t]));
    }
    auto const [graphics_command_buffer, graphics_sync_idx] =
        get_command_buffer(vk::PipelineBindPoint::eGraphics);
    auto const [compute_command_buffer, compute_sync_idx] =
        get_command_buffer(vk::PipelineBindPoint::eCompute);
    megakernel_raytracer.tlas_buffer = create_gpu_only_buffer(vma_alloc,
        size_in_byte(bvh.tlas), {}, vk::BufferUsageFlagBits::eStorageBuffer);
    megakernel_raytracer.blas_buffer = create_gpu_only_buffer(vma_alloc,
        size_in_byte(bvh.blas), {}, vk::BufferUsageFlagBits::eStorageBuffer);
    megakernel_raytracer.mesh_buffer = create_gpu_only_buffer(vma_alloc,
        size_in_byte(bvh.meshes), {}, vk::BufferUsageFlagBits::eStorageBuffer);
    megakernel_raytracer.transform_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(scene.transformation),
            {}, vk::BufferUsageFlagBits::eStorageBuffer);
    megakernel_raytracer.inverse_transform_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(inverse_transformations),
            {}, vk::BufferUsageFlagBits::eStorageBuffer);
    megakernel_raytracer.instance_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(bvh.instances), {},
            vk::BufferUsageFlagBits::eStorageBuffer);
    megakernel_raytracer.triangle_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(bvh.triangles), {},
            vk::BufferUsageFlagBits::eStorageBuffer);
    megakernel_raytracer.material_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(scene.materials), {},
            vk::BufferUsageFlagBits::eStorageBuffer);
    megakernel_raytracer.medium_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(scene.mediums), {},
            vk::BufferUsageFlagBits::eStorageBuffer);
    megakernel_raytracer.light_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(scene.lights), {},
            vk::BufferUsageFlagBits::eStorageBuffer);
    megakernel_raytracer.preview_image = create_texture2d(device, vma_alloc,
        compute_command_buffer, preview_width, preview_height, 1,
        vk::Format::eR32G32B32A32Sfloat,
        {command_queues.graphics_queue_idx, command_queues.compute_queue_idx},
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage);
    megakernel_raytracer.accumulation_image = create_texture2d(device,
        vma_alloc, compute_command_buffer, swapchain_extent.width,
        swapchain_extent.height, 1, vk::Format::eR32G32B32A32Sfloat, {},
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst);
    for (uint32_t t = 0; t < scene.textures.size(); ++t) {
        texture_data const& data = scene.textures[t];
        vk::Format const format = data.format == texture_format::unorm ?
                                      vk::Format::eR8G8B8Unorm :
                                      vk::Format::eR32G32B32Sfloat;
        megakernel_raytracer.texture_array.push_back(
            create_texture2d(device, vma_alloc, graphics_command_buffer,
                data.width, data.height, 1, format, {},
                vk::ImageUsageFlagBits::eSampled |
                    vk::ImageUsageFlagBits::eTransferDst));
    }
    megakernel_raytracer.output_image = create_texture2d(device, vma_alloc,
        graphics_command_buffer, swapchain_extent.width,
        swapchain_extent.height, 1, vk::Format::eR32G32B32A32Sfloat, {},
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eTransferDst);
    update_buffer(vma_alloc, compute_command_buffer,
        megakernel_raytracer.tlas_buffer, to_byte_span(bvh.tlas), 0);
    update_buffer(vma_alloc, compute_command_buffer,
        megakernel_raytracer.blas_buffer, to_byte_span(bvh.blas), 0);
    update_buffer(vma_alloc, compute_command_buffer,
        megakernel_raytracer.mesh_buffer, to_byte_span(bvh.meshes), 0);
    update_buffer(vma_alloc, compute_command_buffer,
        megakernel_raytracer.transform_buffer,
        to_byte_span(scene.transformation), 0);
    update_buffer(vma_alloc, compute_command_buffer,
        megakernel_raytracer.inverse_transform_buffer,
        to_byte_span(inverse_transformations), 0);
    update_buffer(vma_alloc, compute_command_buffer,
        megakernel_raytracer.instance_buffer, to_byte_span(bvh.instances), 0);
    update_buffer(vma_alloc, compute_command_buffer,
        megakernel_raytracer.triangle_buffer, to_byte_span(bvh.triangles), 0);
    update_buffer(vma_alloc, compute_command_buffer,
        megakernel_raytracer.material_buffer, to_byte_span(scene.materials), 0);
    update_buffer(vma_alloc, compute_command_buffer,
        megakernel_raytracer.medium_buffer, to_byte_span(scene.mediums), 0);
    update_buffer(vma_alloc, compute_command_buffer,
        megakernel_raytracer.light_buffer, to_byte_span(scene.lights), 0);
    for (uint32_t t = 0; t < megakernel_raytracer.texture_array.size(); ++t) {
        update_texture2d(vma_alloc, megakernel_raytracer.texture_array[t],
            graphics_command_buffer, scene.textures[t]);
    }
    // buffer barrier
    std::array const buffers{
        megakernel_raytracer.tlas_buffer,
        megakernel_raytracer.blas_buffer,
        megakernel_raytracer.mesh_buffer,
        megakernel_raytracer.transform_buffer,
        megakernel_raytracer.inverse_transform_buffer,
        megakernel_raytracer.instance_buffer,
        megakernel_raytracer.triangle_buffer,
        megakernel_raytracer.material_buffer,
        megakernel_raytracer.medium_buffer,
        megakernel_raytracer.light_buffer,
    };
    std::array<vk::BufferMemoryBarrier, buffers.size()> buffer_barriers{};
    for (uint32_t i = 0; i < buffers.size(); ++i) {
        buffer_barriers[i] = vk::BufferMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = buffers[i].buffer,
            .offset = 0,
            .size = vk::WholeSize,
        };
    }
    // image barrier
    std::vector<vk::ImageMemoryBarrier> image_barriers{};
    image_barriers.reserve(megakernel_raytracer.texture_array.size());
    for (uint32_t t = 0; t < megakernel_raytracer.texture_array.size(); ++t) {
        image_barriers.push_back(vk::ImageMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead,
            .oldLayout = vk::ImageLayout::eGeneral,
            .newLayout = vk::ImageLayout::eGeneral,
            .srcQueueFamilyIndex = command_queues.graphics_queue_idx,
            .dstQueueFamilyIndex = command_queues.compute_queue_idx,
            .image = megakernel_raytracer.texture_array[t].image,
            .subresourceRange = vk::ImageSubresourceRange{
                                                          .aspectMask = vk::ImageAspectFlagBits::eColor,
                                                          .baseMipLevel = 0,
                                                          .levelCount = megakernel_raytracer.texture_array[t].level,
                                                          .baseArrayLayer = 0,
                                                          .layerCount = megakernel_raytracer.texture_array[t].layer}
        });
    }
    compute_command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr,
        (uint32_t) buffer_barriers.size(), buffer_barriers.data(),
        (uint32_t) image_barriers.size(), image_barriers.data());
    for (uint32_t f = 0; f < FRAME_IN_FLIGHT; ++f) {
        // set 0
        update_descriptor_storage_buffer_whole(device,
            megakernel_raytracer.descriptor_sets[0][f], 0, 0,
            megakernel_raytracer.tlas_buffer);
        update_descriptor_storage_buffer_whole(device,
            megakernel_raytracer.descriptor_sets[0][f], 1, 0,
            megakernel_raytracer.blas_buffer);
        // set 1
        update_descriptor_storage_buffer_whole(device,
            megakernel_raytracer.descriptor_sets[1][f], 0, 0,
            megakernel_raytracer.mesh_buffer);
        update_descriptor_storage_buffer_whole(device,
            megakernel_raytracer.descriptor_sets[1][f], 1, 0,
            megakernel_raytracer.transform_buffer);
        update_descriptor_storage_buffer_whole(device,
            megakernel_raytracer.descriptor_sets[1][f], 2, 0,
            megakernel_raytracer.inverse_transform_buffer);
        update_descriptor_storage_buffer_whole(device,
            megakernel_raytracer.descriptor_sets[1][f], 3, 0,
            megakernel_raytracer.instance_buffer);
        update_descriptor_storage_buffer_whole(device,
            megakernel_raytracer.descriptor_sets[1][f], 4, 0,
            megakernel_raytracer.triangle_buffer);
        // set 2
        update_descriptor_storage_buffer_whole(device,
            megakernel_raytracer.descriptor_sets[2][f], 0, 0,
            megakernel_raytracer.material_buffer);
        update_descriptor_storage_buffer_whole(device,
            megakernel_raytracer.descriptor_sets[2][f], 1, 0,
            megakernel_raytracer.medium_buffer);
        update_descriptor_storage_buffer_whole(device,
            megakernel_raytracer.descriptor_sets[2][f], 2, 0,
            megakernel_raytracer.light_buffer);
        // set 3
        update_descriptor_storage_image(device,
            megakernel_raytracer.descriptor_sets[3][f], 0, 0,
            megakernel_raytracer.accumulation_image.primary_view);
        update_descriptor_storage_image(device,
            megakernel_raytracer.descriptor_sets[3][f], 0, 1,
            megakernel_raytracer.preview_image.primary_view);
        for (uint32_t t = 0; t < megakernel_raytracer.texture_array.size();
             ++t) {
            update_descriptor_image_sampler_combined(device,
                megakernel_raytracer.descriptor_sets[3][f], 1, t,
                primary_sampler,
                megakernel_raytracer.texture_array[t].primary_view);
        }
    }
}

static void clean_megakernel_raytracer_resources() {
    destroy_buffer(vma_alloc, megakernel_raytracer.tlas_buffer);
    destroy_buffer(vma_alloc, megakernel_raytracer.blas_buffer);
    destroy_buffer(vma_alloc, megakernel_raytracer.mesh_buffer);
    destroy_buffer(vma_alloc, megakernel_raytracer.transform_buffer);
    destroy_buffer(vma_alloc, megakernel_raytracer.inverse_transform_buffer);
    destroy_buffer(vma_alloc, megakernel_raytracer.instance_buffer);
    destroy_buffer(vma_alloc, megakernel_raytracer.triangle_buffer);
    destroy_buffer(vma_alloc, megakernel_raytracer.material_buffer);
    destroy_buffer(vma_alloc, megakernel_raytracer.medium_buffer);
    destroy_buffer(vma_alloc, megakernel_raytracer.light_buffer);
    destroy_image(device, vma_alloc, megakernel_raytracer.accumulation_image);
    destroy_image(device, vma_alloc, megakernel_raytracer.preview_image);
    destroy_image(device, vma_alloc, megakernel_raytracer.output_image);
    for (uint32_t i = 0; i < megakernel_raytracer.texture_array.size(); ++i) {
        destroy_image(device, vma_alloc, megakernel_raytracer.texture_array[i]);
    }
    megakernel_raytracer.texture_array.clear();
}

static void create_rect_pipeline() {
    std::vector<vk_descriptor_set_binding> bindings{
        {vk::DescriptorType::eCombinedImageSampler, 2}
    };
    rect.descriptor_layout = create_descriptor_set_layout(
        device, vk::ShaderStageFlagBits::eFragment, bindings);
    create_descriptor_set(device, primary_descriptor_pool,
        rect.descriptor_layout, rect.descriptor_sets);
    std::array pc_sizes{(uint32_t) sizeof(rect_pc)};
    std::array pc_stages{vk::ShaderStageFlagBits::eFragment};
    std::array layouts{rect.descriptor_layout};
    rect.pipeline_layout =
        create_pipeline_layout(device, pc_sizes, pc_stages, layouts);
    rect.pipeline = create_graphics_pipeline(device,
        PATH_FROM_BINARY("shaders/rect.vert.spv"),
        PATH_FROM_BINARY("shaders/rect.frag.spv"), rect.pipeline_layout,
        frame_objects.render_pass, {}, vk::PolygonMode::eFill, false, false);
}

static void destroy_rect_pipeline() {
    device.destroyDescriptorSetLayout(rect.descriptor_layout);
    device.destroyPipelineLayout(rect.pipeline_layout);
    device.destroy(rect.pipeline);
}

static void prepare_rect_resources() {
    for (uint32_t f = 0; f < FRAME_IN_FLIGHT; ++f) {
        update_descriptor_image_sampler_combined(device,
            rect.descriptor_sets[f], 0, 0, primary_sampler,
            megakernel_raytracer.output_image.primary_view);
        update_descriptor_image_sampler_combined(device,
            rect.descriptor_sets[f], 0, 1, blocky_sampler,
            megakernel_raytracer.preview_image.primary_view);
    }
}

void megakernel_raytracer_initialize(render_options const& options) {
    if (initialized) {
        return;
    }
    initialized = true;
    tiles.count.x = options.resolution_x / options.tile_width;
    tiles.count.y = options.resolution_y / options.tile_height;
    tiles.size.x = options.tile_width;
    tiles.size.y = options.tile_height;
    max_tracing_depth = options.max_depth;
    primary_descriptor_pool = create_descriptor_pool(device);
    indexing_descriptor_pool = create_descriptor_pool(
        device, vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind);
    primary_sampler = create_default_sampler(device);
    blocky_sampler = create_blocky_sampler(device);
    vk::SemaphoreCreateInfo const semaphore_info{};
    vk::Result result;
    for (uint32_t f = 0; f < FRAME_IN_FLIGHT; ++f) {
        VK_CHECK_CREATE(result, compute_semaphores[f],
            device.createSemaphore(semaphore_info));
        VK_CHECK_CREATE(result, graphics_semaphores[f],
            device.createSemaphore(semaphore_info));
        VK_CHECK_CREATE(result, present_semaphores[f],
            device.createSemaphore(semaphore_info));
    }
    create_frame_objects();
    create_megakernel_raytracer_pipeline();
    create_rect_pipeline();
}

void megakernel_raytracer_prepare_data(scene const& scene) {
    preview_width = win_width / PREVIEW_RATIO;
    preview_height = win_height / PREVIEW_RATIO;
    prepare_megakernel_raytracer_resources(scene);
    prepare_rect_resources();
}

void megakernel_raytracer_update_data(scene const&) {
}

void megakernel_raytracer_render(camera const& camera) {
    fmt::println("\rAccumualtion count: {}", accumulation_counter);
    vk::Result result;
    auto const [compute_command_buffer, compute_sync_idx] =
        get_command_buffer(vk::PipelineBindPoint::eCompute);
    auto const [graphics_command_buffer, graphics_sync_idx] =
        get_command_buffer(vk::PipelineBindPoint::eGraphics);
    result = swapchain_acquire_next_image_wrapper(device,
        frame_objects.swapchain, 1e9, present_semaphores[graphics_sync_idx],
        nullptr, &frame_objects.swapchain_image_idx);
    if (result == vk::Result::eErrorOutOfDateKHR) {
        refresh_frame_objects();
        return;
    }
    CHECK(
        result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR,
        "");
    // megakernel raytracer
    compute_command_buffer.bindPipeline(
        vk::PipelineBindPoint ::eCompute, megakernel_raytracer.pipeline);
    std::array<vk::DescriptorSet, MEGAKERNAL_RAYTRACER_SET>
        megakernel_raytracer_sets{};
    for (uint32_t s = 0; s < MEGAKERNAL_RAYTRACER_SET; ++s) {
        megakernel_raytracer_sets[s] =
            megakernel_raytracer.descriptor_sets[s][compute_sync_idx];
    }
    compute_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
        megakernel_raytracer.pipeline_layout, 0, MEGAKERNAL_RAYTRACER_SET,
        megakernel_raytracer_sets.data(), 0, nullptr);
    vk::ImageSubresourceRange const whole_range{
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    if (camera.dirty) {
        bool const camera_start_moving = accumulation_counter != 0;
        accumulation_counter = 0;
        tiles.current.x = 0;
        tiles.current.y = 0;
        megakernel_raytracer_pc const megakernel_raytracer_pc{
            .camera = get_glsl_raytracer_camera(
                camera, preview_width, preview_height),
            .random_seed = rand_uint(),
            .preview = 1,
            .max_depth = max_tracing_depth,
            .sky_light = sky_light_idx,
        };
        compute_command_buffer.pushConstants(
            megakernel_raytracer.pipeline_layout,
            vk::ShaderStageFlagBits::eCompute, 0,
            (uint32_t) sizeof(megakernel_raytracer_pc),
            &megakernel_raytracer_pc);
        compute_command_buffer.dispatch(preview_width, preview_height, 1);
        if (camera_start_moving) {
            add_submit_signal(vk::PipelineBindPoint::eCompute,
                compute_semaphores[compute_sync_idx]);
            add_submit_wait(vk::PipelineBindPoint::eGraphics,
                compute_semaphores[compute_sync_idx],
                vk::PipelineStageFlagBits::eFragmentShader);
        }
    } else {
        if (accumulation_counter == 0 && tiles.current.x == 0 &&
            tiles.current.y == 0) {
            std::array const black{0.0f, 0.0f, 0.0f, 1.0f};
            vk::ClearColorValue const black_clear{.float32 = black};
            compute_command_buffer.clearColorImage(
                megakernel_raytracer.accumulation_image.image,
                vk::ImageLayout::eGeneral, &black_clear, 1, &whole_range);
            vk::ImageMemoryBarrier const clear_barrier{
                .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
                .dstAccessMask = vk::AccessFlagBits::eShaderRead,
                .oldLayout = vk::ImageLayout::eGeneral,
                .newLayout = vk::ImageLayout::eGeneral,
                .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                .image = megakernel_raytracer.accumulation_image.image,
                .subresourceRange = whole_range,
            };
            compute_command_buffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0,
                nullptr, 1, &clear_barrier);
        }
        megakernel_raytracer_pc const megakernel_raytracer_pc{
            .camera = get_glsl_raytracer_camera(
                camera, swapchain_extent.width, swapchain_extent.height),
            .random_seed = rand_uint(),
            .preview = 0,
            .max_depth = max_tracing_depth,
            .sky_light = sky_light_idx,
        };
        compute_command_buffer.pushConstants(
            megakernel_raytracer.pipeline_layout,
            vk::ShaderStageFlagBits::eCompute, 0,
            (uint32_t) sizeof(megakernel_raytracer_pc),
            &megakernel_raytracer_pc);
        glm::uvec2 const viewport = current_viewport();
        compute_command_buffer.dispatchBase(
            viewport.x, viewport.y, 0, tiles.size.x, tiles.size.y, 1);
        bool const finished = next_tile();
        if (finished) {
            ++accumulation_counter;
            vk::ImageMemoryBarrier const render_barrier{
                .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
                .dstAccessMask = vk::AccessFlagBits::eTransferRead,
                .oldLayout = vk::ImageLayout::eGeneral,
                .newLayout = vk::ImageLayout::eGeneral,
                .srcQueueFamilyIndex = command_queues.compute_queue_idx,
                .dstQueueFamilyIndex = command_queues.compute_queue_idx,
                .image = megakernel_raytracer.accumulation_image.image,
                .subresourceRange = whole_range,
            };
            compute_command_buffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eComputeShader,
                vk::PipelineStageFlagBits::eTransfer, {}, 0, nullptr, 0,
                nullptr, 1, &render_barrier);
            vk::ImageSubresourceLayers const layer{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };
            vk::Offset3D const offset{0, 0, 0};
            vk::Extent3D const extent{
                megakernel_raytracer.accumulation_image.width,
                megakernel_raytracer.accumulation_image.height,
                1,
            };
            vk::ImageCopy const image_copy{
                .srcSubresource = layer,
                .srcOffset = offset,
                .dstSubresource = layer,
                .dstOffset = offset,
                .extent = extent,
            };
            compute_command_buffer.copyImage(
                megakernel_raytracer.accumulation_image.image,
                vk::ImageLayout::eGeneral,
                megakernel_raytracer.output_image.image,
                vk::ImageLayout::eGeneral, 1, &image_copy);
        }
    }
    // rect
    vk::Rect2D const render_area{
        .offset = {0, 0},
        .extent = swapchain_extent,
    };
    vk::ClearValue const color_clear{
        .color = {std::array{0.0f, 0.0f, 0.0f, 1.0f}}};
    vk::RenderPassBeginInfo const render_pass_begin_info{
        .renderPass = frame_objects.render_pass,
        .framebuffer =
            frame_objects.framebuffers[frame_objects.swapchain_image_idx],
        .renderArea = render_area,
        .clearValueCount = 1,
        .pClearValues = &color_clear,
    };
    graphics_command_buffer.beginRenderPass(
        render_pass_begin_info, vk::SubpassContents::eInline);
    vk::Viewport const viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) swapchain_extent.width,
        .height = (float) swapchain_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    graphics_command_buffer.setViewport(0, 1, &viewport);
    vk::Rect2D const scissor = render_area;
    graphics_command_buffer.setScissor(0, 1, &scissor);
    graphics_command_buffer.bindPipeline(
        vk::PipelineBindPoint::eGraphics, rect.pipeline);
    graphics_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        rect.pipeline_layout, 0, 1, &rect.descriptor_sets[graphics_sync_idx], 0,
        nullptr);
    bool const only_preview = accumulation_counter == 0;
    rect_pc const rect_pc{
        .frame_scalar =
            only_preview ? 1.0f : 1.0f / (float) accumulation_counter,
        .preview = only_preview,
    };
    graphics_command_buffer.pushConstants(rect.pipeline_layout,
        vk::ShaderStageFlagBits::eFragment, 0, (uint32_t) sizeof(rect_pc),
        &rect_pc);
    graphics_command_buffer.draw(3, 1, 0, 0);
    graphics_command_buffer.endRenderPass();
    if (camera.dirty) {
        vk::ImageMemoryBarrier const preview_barrier{
            .srcAccessMask = vk::AccessFlagBits::eNone,
            .dstAccessMask = vk::AccessFlagBits::eNone,
            .oldLayout = vk::ImageLayout::eGeneral,
            .newLayout = vk::ImageLayout::eGeneral,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = megakernel_raytracer.preview_image.image,
            .subresourceRange = whole_range,
        };
        compute_command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eNone,
            vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0,
            nullptr, 1, &preview_barrier);
    }
    add_submit_wait(vk::PipelineBindPoint::eGraphics,
        present_semaphores[graphics_sync_idx],
        vk::PipelineStageFlagBits::eColorAttachmentOutput);
    add_submit_signal(vk::PipelineBindPoint::eGraphics,
        graphics_semaphores[graphics_sync_idx]);
    add_present_wait(graphics_semaphores[graphics_sync_idx]);
}

void megakernel_raytracer_present() {
    vk::Result const result =
        present(frame_objects.swapchain, frame_objects.swapchain_image_idx);
    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
        refresh_frame_objects();
    } else {
        CHECK(result == vk::Result::eSuccess, "");
    }
}

void megakernel_raytracer_destroy() {
    if (!initialized) {
        return;
    }
    initialized = false;
    device.destroyDescriptorPool(primary_descriptor_pool);
    device.destroyDescriptorPool(indexing_descriptor_pool);
    device.destroySampler(primary_sampler);
    device.destroySampler(blocky_sampler);
    for (uint32_t f = 0; f < FRAME_IN_FLIGHT; ++f) {
        device.destroySemaphore(compute_semaphores[f]);
        device.destroySemaphore(graphics_semaphores[f]);
        device.destroySemaphore(present_semaphores[f]);
    }
    cleanup_staging_buffer(vma_alloc);
    cleanup_staging_image(vma_alloc);
    destroy_rect_pipeline();
    clean_megakernel_raytracer_resources();
    destroy_megakernel_raytracer_pipeline();
    destroy_frame_objects();
}

void load_megakernel_raytracer(renderer& renderer) {
    renderer.initialize = megakernel_raytracer_initialize;
    renderer.prepare_data = megakernel_raytracer_prepare_data;
    renderer.update_data = megakernel_raytracer_update_data;
    renderer.render = megakernel_raytracer_render;
    renderer.present = megakernel_raytracer_present;
    renderer.destroy = megakernel_raytracer_destroy;
}
