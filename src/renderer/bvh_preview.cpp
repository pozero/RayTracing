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

void bvh_preview_initialize(render_options const& options);
void bvh_preview_prepare_data(scene const& scene);
void bvh_preview_update_data(scene const& scene);
void bvh_preview_render(camera const& camera);
void bvh_preview_present();
void bvh_preview_destroy();

static void create_frame_objects();
static void destroy_frame_objects();
static void refresh_frame_objects();
static void create_bvh_preview_pipeline();
static void destroy_bvh_preview_pipeline();
static void prepare_bvh_preview_resources(scene const& scene);
static void clean_bvh_preview_resources();
static void create_rect_pipeline();
static void destroy_rect_pipeline();
static void prepare_rect_resources();

static bool initialized = false;

static vk::DescriptorPool primary_descriptor_pool{};
static vk::Sampler primary_sampler{};
static std::array<vk::Semaphore, FRAME_IN_FLIGHT> compute_semaphores{};
static std::array<vk::Semaphore, FRAME_IN_FLIGHT> graphics_semaphores{};
static std::array<vk::Semaphore, FRAME_IN_FLIGHT> present_semaphores{};

static struct {
    vk::SwapchainKHR swapchain;
    std::vector<vk::ImageView> presents;
    vk::RenderPass render_pass;
    std::vector<vk::Framebuffer> framebuffers;
    uint32_t swapchain_image_idx;
} frame_objects{};

static uint32_t constexpr BVH_PREVIEW_SET = 3;

static struct {
    // descriptors
    std::array<vk::DescriptorSetLayout, BVH_PREVIEW_SET> descriptor_layouts;
    std::array<std::array<vk::DescriptorSet, FRAME_IN_FLIGHT>, BVH_PREVIEW_SET>
        descriptor_sets;
    // pipeline
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;
    // resources
    // set 0 resources
    vk_buffer tlas_buffer;
    vk_buffer blas_buffer;
    // set 1 resources
    vk_buffer mesh_buffer;
    vk_buffer inverse_transform_buffer;
    vk_buffer instance_buffer;
    // set 2 resources
    vk_image output_image;
} bvh_preview;

struct bvh_preview_pc {
    glsl_raytracer_camera camera;
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
};

static void create_frame_objects() {
    std::tie(frame_objects.swapchain, frame_objects.presents) =
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
    std::tie(frame_objects.swapchain, frame_objects.presents) =
        create_swapchain(device, surface, command_queues);
    frame_objects.framebuffers = create_framebuffers(
        device, frame_objects.render_pass, frame_objects.presents);
}

static void create_bvh_preview_pipeline() {
    std::array const bindings{
        std::vector<vk_descriptor_set_binding>{
                                               {vk::DescriptorType::eStorageBuffer, 1},
                                               {vk::DescriptorType::eStorageBuffer, 1}},
        std::vector<vk_descriptor_set_binding>{
                                               {vk::DescriptorType::eStorageBuffer, 1},
                                               {vk::DescriptorType::eStorageBuffer, 1},
                                               {vk::DescriptorType::eStorageBuffer, 1}},
        std::vector<vk_descriptor_set_binding>{
                                               {vk::DescriptorType::eStorageImage, 1}},
    };
    for (uint32_t s = 0; s < BVH_PREVIEW_SET; ++s) {
        bvh_preview.descriptor_layouts[s] = create_descriptor_set_layout(
            device, vk::ShaderStageFlagBits::eCompute, bindings[s]);
        create_descriptor_set(device, primary_descriptor_pool,
            bvh_preview.descriptor_layouts[s], bvh_preview.descriptor_sets[s]);
    }
    std::array pc_sizes{(uint32_t) sizeof(bvh_preview_pc)};
    std::array pc_stages{vk::ShaderStageFlagBits::eCompute};
    bvh_preview.pipeline_layout = create_pipeline_layout(
        device, pc_sizes, pc_stages, bvh_preview.descriptor_layouts);
    bvh_preview.pipeline = create_compute_pipeline(device,
        PATH_FROM_BINARY("shaders/bvh_preview.comp.spv"),
        bvh_preview.pipeline_layout, {});
}

static void destroy_bvh_preview_pipeline() {
    for (uint32_t s = 0; s < BVH_PREVIEW_SET; ++s) {
        device.destroyDescriptorSetLayout(bvh_preview.descriptor_layouts[s]);
    }
    device.destroyPipelineLayout(bvh_preview.pipeline_layout);
    device.destroyPipeline(bvh_preview.pipeline);
}

static void prepare_bvh_preview_resources(scene const& scene) {
    clean_bvh_preview_resources();
    bvh const bvh = create_bvh(scene);
    std::vector<glm::mat4> inverse_transformations{};
    inverse_transformations.reserve(scene.transformation.size());
    for (uint32_t t = 0; t < scene.transformation.size(); ++t) {
        inverse_transformations.push_back(
            glm::inverse(scene.transformation[t]));
    }
    auto const [command_buffer, sync_idx] =
        get_command_buffer(vk::PipelineBindPoint::eCompute);
    bvh_preview.tlas_buffer = create_gpu_only_buffer(vma_alloc,
        size_in_byte(bvh.tlas), {}, vk::BufferUsageFlagBits::eStorageBuffer);
    bvh_preview.blas_buffer = create_gpu_only_buffer(vma_alloc,
        size_in_byte(bvh.blas), {}, vk::BufferUsageFlagBits::eStorageBuffer);
    bvh_preview.mesh_buffer = create_gpu_only_buffer(vma_alloc,
        size_in_byte(bvh.meshes), {}, vk::BufferUsageFlagBits::eStorageBuffer);
    bvh_preview.inverse_transform_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(inverse_transformations),
            {}, vk::BufferUsageFlagBits::eStorageBuffer);
    bvh_preview.instance_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(bvh.instances), {},
            vk::BufferUsageFlagBits::eStorageBuffer);
    bvh_preview.output_image = create_texture2d(device, vma_alloc,
        command_buffer, win_width, win_height, 1,
        vk::Format::eR32G32B32A32Sfloat,
        {command_queues.compute_queue_idx, command_queues.graphics_queue_idx},
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage);
    update_buffer(vma_alloc, command_buffer, bvh_preview.tlas_buffer,
        to_byte_span(bvh.tlas), 0);
    update_buffer(vma_alloc, command_buffer, bvh_preview.blas_buffer,
        to_byte_span(bvh.blas), 0);
    update_buffer(vma_alloc, command_buffer, bvh_preview.mesh_buffer,
        to_byte_span(bvh.meshes), 0);
    update_buffer(vma_alloc, command_buffer,
        bvh_preview.inverse_transform_buffer,
        to_byte_span(inverse_transformations), 0);
    update_buffer(vma_alloc, command_buffer, bvh_preview.instance_buffer,
        to_byte_span(bvh.instances), 0);
    // set 0 barrier
    std::array const set_0_bufs{
        bvh_preview.tlas_buffer,
        bvh_preview.blas_buffer,
    };
    std::array<vk::BufferMemoryBarrier, set_0_bufs.size()> set_0_barriers{};
    for (uint32_t i = 0; i < set_0_bufs.size(); ++i) {
        set_0_barriers[i] = vk::BufferMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = set_0_bufs[i].buffer,
            .offset = 0,
            .size = vk::WholeSize,
        };
    }
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr,
        (uint32_t) set_0_barriers.size(), set_0_barriers.data(), 0, nullptr);
    // set 1 barrier
    std::array const set_1_bufs{
        bvh_preview.tlas_buffer,
        bvh_preview.blas_buffer,
    };
    std::array<vk::BufferMemoryBarrier, set_1_bufs.size()> set_1_barriers{};
    for (uint32_t i = 0; i < set_1_bufs.size(); ++i) {
        set_1_barriers[i] = vk::BufferMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = set_0_bufs[i].buffer,
            .offset = 0,
            .size = vk::WholeSize,
        };
    }
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr,
        (uint32_t) set_1_barriers.size(), set_1_barriers.data(), 0, nullptr);
    for (uint32_t f = 0; f < FRAME_IN_FLIGHT; ++f) {
        // set 0
        update_descriptor_storage_buffer_whole(device,
            bvh_preview.descriptor_sets[0][f], 0, 0, bvh_preview.tlas_buffer);
        update_descriptor_storage_buffer_whole(device,
            bvh_preview.descriptor_sets[0][f], 1, 0, bvh_preview.blas_buffer);
        // set 1
        update_descriptor_storage_buffer_whole(device,
            bvh_preview.descriptor_sets[1][f], 0, 0, bvh_preview.mesh_buffer);
        update_descriptor_storage_buffer_whole(device,
            bvh_preview.descriptor_sets[1][f], 1, 0,
            bvh_preview.inverse_transform_buffer);
        update_descriptor_storage_buffer_whole(device,
            bvh_preview.descriptor_sets[1][f], 2, 0,
            bvh_preview.instance_buffer);
        // set 2
        update_descriptor_storage_image(device,
            bvh_preview.descriptor_sets[2][f], 0, 0,
            bvh_preview.output_image.primary_view);
    }
}

static void create_rect_pipeline() {
    std::vector<vk_descriptor_set_binding> bindings{
        {vk::DescriptorType::eCombinedImageSampler, 1}
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
            bvh_preview.output_image.primary_view);
    }
}

static void clean_bvh_preview_resources() {
    destroy_buffer(vma_alloc, bvh_preview.tlas_buffer);
    destroy_buffer(vma_alloc, bvh_preview.blas_buffer);
    destroy_buffer(vma_alloc, bvh_preview.mesh_buffer);
    destroy_buffer(vma_alloc, bvh_preview.inverse_transform_buffer);
    destroy_buffer(vma_alloc, bvh_preview.instance_buffer);
    destroy_image(device, vma_alloc, bvh_preview.output_image);
}

void load_bvh_preview(renderer& renderer) {
    renderer.initialize = bvh_preview_initialize;
    renderer.prepare_data = bvh_preview_prepare_data;
    renderer.update_data = bvh_preview_update_data;
    renderer.render = bvh_preview_render;
    renderer.present = bvh_preview_present;
    renderer.destroy = bvh_preview_destroy;
}

void bvh_preview_initialize(render_options const&) {
    if (initialized) {
        return;
    }
    initialized = true;
    vk::Result result;
    primary_descriptor_pool = create_descriptor_pool(
        device, vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind);
    primary_sampler = create_default_sampler(device);
    vk::SemaphoreCreateInfo const semaphore_info{};
    for (uint32_t f = 0; f < FRAME_IN_FLIGHT; ++f) {
        VK_CHECK_CREATE(result, compute_semaphores[f],
            device.createSemaphore(semaphore_info));
        VK_CHECK_CREATE(result, graphics_semaphores[f],
            device.createSemaphore(semaphore_info));
        VK_CHECK_CREATE(result, present_semaphores[f],
            device.createSemaphore(semaphore_info));
    }
    create_frame_objects();
    create_bvh_preview_pipeline();
    create_rect_pipeline();
}

void bvh_preview_prepare_data(scene const& scene) {
    prepare_bvh_preview_resources(scene);
    prepare_rect_resources();
}

void bvh_preview_update_data(scene const&) {
}

void bvh_preview_render(camera const& camera) {
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
    // bvh preview
    bvh_preview_pc const bvh_preview_pc{
        .camera = get_glsl_raytracer_camera(camera, win_width, win_height),
    };
    compute_command_buffer.bindPipeline(
        vk::PipelineBindPoint::eCompute, bvh_preview.pipeline);
    std::array<vk::DescriptorSet, BVH_PREVIEW_SET> bvh_preview_sets{};
    for (uint32_t s = 0; s < BVH_PREVIEW_SET; ++s) {
        bvh_preview_sets[s] = bvh_preview.descriptor_sets[s][compute_sync_idx];
    }
    compute_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
        bvh_preview.pipeline_layout, 0, BVH_PREVIEW_SET,
        bvh_preview_sets.data(), 0, nullptr);
    compute_command_buffer.pushConstants(bvh_preview.pipeline_layout,
        vk::ShaderStageFlagBits::eCompute, 0, (uint32_t) sizeof(bvh_preview_pc),
        &bvh_preview_pc);
    compute_command_buffer.dispatch(win_width, win_height, 1);
    add_submit_signal(
        vk::PipelineBindPoint::eCompute, compute_semaphores[compute_sync_idx]);
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
    rect_pc const rect_pc{.frame_scalar = 1.0f};
    graphics_command_buffer.pushConstants(rect.pipeline_layout,
        vk::ShaderStageFlagBits::eFragment, 0, (uint32_t) sizeof(rect_pc),
        &rect_pc);
    graphics_command_buffer.draw(3, 1, 0, 0);
    graphics_command_buffer.endRenderPass();
    add_submit_wait(vk::PipelineBindPoint::eGraphics,
        present_semaphores[graphics_sync_idx],
        vk::PipelineStageFlagBits::eColorAttachmentOutput);
    add_submit_wait(vk::PipelineBindPoint::eGraphics,
        compute_semaphores[graphics_sync_idx],
        vk::PipelineStageFlagBits::eFragmentShader);
    add_submit_signal(vk::PipelineBindPoint::eGraphics,
        graphics_semaphores[graphics_sync_idx]);
    add_present_wait(graphics_semaphores[graphics_sync_idx]);
}

void bvh_preview_present() {
    vk::Result const result =
        present(frame_objects.swapchain, frame_objects.swapchain_image_idx);
    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
        refresh_frame_objects();
    } else {
        CHECK(result == vk::Result::eSuccess, "");
    }
}

void bvh_preview_destroy() {
    if (!initialized) {
        return;
    }
    device.destroyDescriptorPool(primary_descriptor_pool);
    device.destroySampler(primary_sampler);
    for (uint32_t f = 0; f < FRAME_IN_FLIGHT; ++f) {
        device.destroySemaphore(compute_semaphores[f]);
        device.destroySemaphore(graphics_semaphores[f]);
        device.destroySemaphore(present_semaphores[f]);
    }
    cleanup_staging_buffer(vma_alloc);
    cleanup_staging_image(vma_alloc);
    destroy_rect_pipeline();
    clean_bvh_preview_resources();
    destroy_bvh_preview_pipeline();
    destroy_frame_objects();
    initialized = false;
}
