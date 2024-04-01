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

#include "utils/to_span.h"
#include "utils/file.h"

#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"

void rasterizer_initialize(render_options const& options);
void rasterizer_prepare_data(scene const& scene);
void rasterizer_update_data(scene const& scene);
void rasterizer_render(camera const& camera);
void rasterizer_present();
void rasterizer_destroy();

struct glsl_instance {
    int32_t transform;
    int32_t material;
    int32_t light;
};

static void create_frame_objects();
static void destroy_frame_objects();
static void refresh_frame_objects();
static void create_rasterization_pipeline();
static void destroy_rasterization_pipeline();
static void prepare_rasterization_resources(scene const& scene);
static void clean_rasterization_resources();

static bool initialized = false;

static vk::DescriptorPool primary_descriptor_pool{};
static vk::Sampler primary_sampler{};
static std::array<vk::Semaphore, FRAME_IN_FLIGHT> graphics_semaphores{};
static std::array<vk::Semaphore, FRAME_IN_FLIGHT> present_semaphores{};

static struct {
    vk::SwapchainKHR swapchain;
    std::vector<vk::ImageView> presents;
    vk_image depth;
    vk_image color;
    vk::RenderPass render_pass;
    std::vector<vk::Framebuffer> framebuffers;
    uint32_t swapchain_image_idx;
} frame_objects{};

static uint32_t constexpr RASTERIZATION_SET = 2;

static struct {
    // descriptors
    std::array<vk::DescriptorSetLayout, RASTERIZATION_SET> descriptor_layouts;
    std::array<std::array<vk::DescriptorSet, FRAME_IN_FLIGHT>,
        RASTERIZATION_SET>
        descriptor_sets;
    // pipeline
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;
    // resources
    vk_buffer indirect_draw_buffer;
    // set 0 resources
    vk_buffer vertices_buffer;
    vk_buffer transformation_buffer;
    vk_buffer instance_buffer;
    // set 1 resources
    vk_buffer material_buffer;
    vk_buffer light_buffer;
    std::vector<vk_image> texture_array;
} rasterization{};

struct rasterization_vert_pc {
    glm::mat4 proj_view;
};

static uint32_t mesh_instance_count = 0;

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

static void create_rasterization_pipeline() {
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
        std::vector<vk_descriptor_set_binding>{single_storage_buffer_binding,
                                               single_storage_buffer_binding, single_storage_buffer_binding},
        std::vector<vk_descriptor_set_binding>{
                                               single_storage_buffer_binding, single_storage_buffer_binding,
                                               texture_array_binding, },
    };
    std::array const set_shader_stage{
        vk::ShaderStageFlagBits::eVertex,
        vk::ShaderStageFlagBits::eFragment,
    };
    for (uint32_t i = 0; i < RASTERIZATION_SET; ++i) {
        rasterization.descriptor_layouts[i] = create_descriptor_set_layout(
            device, set_shader_stage[i], bindings[i],
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);
        create_descriptor_set(device, primary_descriptor_pool,
            rasterization.descriptor_layouts[i],
            rasterization.descriptor_sets[i]);
    }
    std::array pc_sizes{
        (uint32_t) sizeof(rasterization_vert_pc),
    };
    std::array pc_stages{
        vk::ShaderStageFlagBits::eVertex,
    };
    rasterization.pipeline_layout = create_pipeline_layout(
        device, pc_sizes, pc_stages, rasterization.descriptor_layouts);
    rasterization.pipeline = create_graphics_pipeline(device,
        PATH_FROM_BINARY("shaders/rasterize.vert.spv"),
        PATH_FROM_BINARY("shaders/rasterize.frag.spv"),
        rasterization.pipeline_layout, frame_objects.render_pass, {},
        vk::PolygonMode::eFill, true, true);
}

static void destroy_rasterization_pipeline() {
    for (uint32_t i = 0; i < RASTERIZATION_SET; ++i) {
        device.destroy(rasterization.descriptor_layouts[i]);
    }
    device.destroyPipelineLayout(rasterization.pipeline_layout);
    device.destroyPipeline(rasterization.pipeline);
}

static void prepare_rasterization_resources(scene const& scene) {
    clean_rasterization_resources();
    std::vector<vk::DrawIndirectCommand> indirect_draw_command{};
    std::vector<glsl_instance> instances{};
    std::vector<uint32_t> mesh_vertex_count{};
    indirect_draw_command.reserve(scene.primitives.size());
    instances.reserve(scene.primitives.size() + scene.lights.size());
    mesh_vertex_count.reserve(scene.mesh_vertex_start.size());
    for (uint32_t s = 1; s < scene.mesh_vertex_start.size(); ++s) {
        mesh_vertex_count.push_back(
            scene.mesh_vertex_start[s] - scene.mesh_vertex_start[s - 1]);
    }
    mesh_vertex_count.push_back(
        (uint32_t) scene.vertices.size() - scene.mesh_vertex_start.back());
    for (uint32_t p = 0; p < scene.primitives.size(); ++p) {
        primitive const& prim = scene.primitives[p];
        uint32_t const mesh = (uint32_t) prim.mesh;
        indirect_draw_command.push_back(vk::DrawIndirectCommand{
            .vertexCount = mesh_vertex_count[mesh],
            .instanceCount = 1,
            .firstVertex = scene.mesh_vertex_start[mesh],
            .firstInstance = p,
        });
        instances.push_back(glsl_instance{
            .transform = prim.transform,
            .material = prim.material,
            .light = -1,
        });
    }
    for (uint32_t l = 0, i = (uint32_t) instances.size();
         l < scene.lights.size(); ++l) {
        light const& light = scene.lights[l];
        if (light.type == light_type::distant) {
            continue;
        }
        uint32_t const mesh = (uint32_t) light.mesh;
        indirect_draw_command.push_back(vk::DrawIndirectCommand{
            .vertexCount = mesh_vertex_count[mesh],
            .instanceCount = 1,
            .firstVertex = scene.mesh_vertex_start[mesh],
            .firstInstance = i,
        });
        instances.push_back(glsl_instance{
            .transform = light.transform,
            .material = -1,
            .light = (int32_t) l,
        });
        ++i;
    }
    mesh_instance_count = (uint32_t) indirect_draw_command.size();
    auto const [command_buffer, sync_idx] =
        get_command_buffer(vk::PipelineBindPoint::eGraphics);
    rasterization.indirect_draw_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(indirect_draw_command),
            {}, vk::BufferUsageFlagBits::eIndirectBuffer);
    rasterization.vertices_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(scene.vertices), {},
            vk::BufferUsageFlagBits::eStorageBuffer);
    rasterization.transformation_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(scene.transformation),
            {}, vk::BufferUsageFlagBits::eStorageBuffer);
    rasterization.instance_buffer = create_gpu_only_buffer(vma_alloc,
        size_in_byte(instances), {}, vk::BufferUsageFlagBits::eStorageBuffer);
    rasterization.material_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(scene.materials), {},
            vk::BufferUsageFlagBits::eStorageBuffer);
    rasterization.light_buffer =
        create_gpu_only_buffer(vma_alloc, size_in_byte(scene.lights), {},
            vk::BufferUsageFlagBits::eStorageBuffer);
    rasterization.texture_array.reserve(scene.textures.size());
    for (uint32_t t = 0; t < scene.textures.size(); ++t) {
        texture_data const& data = scene.textures[t];
        vk::Format const format = data.format == texture_format::unorm ?
                                      vk::Format::eR8G8B8A8Unorm :
                                      vk::Format::eR32G32B32A32Sfloat;
        rasterization.texture_array.push_back(create_texture2d(device,
            vma_alloc, command_buffer, data.width, data.height, 1, format, {},
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst));
    }
    update_buffer(vma_alloc, command_buffer, rasterization.indirect_draw_buffer,
        to_byte_span(indirect_draw_command), 0);
    update_buffer(vma_alloc, command_buffer, rasterization.vertices_buffer,
        to_byte_span(scene.vertices), 0);
    update_buffer(vma_alloc, command_buffer,
        rasterization.transformation_buffer, to_byte_span(scene.transformation),
        0);
    update_buffer(vma_alloc, command_buffer, rasterization.instance_buffer,
        to_byte_span(instances), 0);
    update_buffer(vma_alloc, command_buffer, rasterization.material_buffer,
        to_byte_span(scene.materials), 0);
    update_buffer(vma_alloc, command_buffer, rasterization.light_buffer,
        to_byte_span(scene.lights), 0);
    for (uint32_t t = 0; t < rasterization.texture_array.size(); ++t) {
        update_texture2d(vma_alloc, rasterization.texture_array[t],
            command_buffer, scene.textures[t]);
    }
    // draw indirect barrier
    vk::BufferMemoryBarrier const indirect_buffer_upload_barrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eIndirectCommandRead,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .buffer = rasterization.indirect_draw_buffer.buffer,
        .offset = 0,
        .size = vk::WholeSize,
    };
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eDrawIndirect, {}, 0, nullptr, 1,
        &indirect_buffer_upload_barrier, 0, nullptr);
    // vertex barrier
    std::array<vk::BufferMemoryBarrier, 3> vertex_buffer_upload_barriers{};
    std::array vertex_storage_buffers{
        rasterization.vertices_buffer,
        rasterization.transformation_buffer,
        rasterization.instance_buffer,
    };
    for (uint32_t i = 0; i < vertex_buffer_upload_barriers.size(); ++i) {
        vertex_buffer_upload_barriers[i] = vk::BufferMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = vertex_storage_buffers[i].buffer,
            .offset = 0,
            .size = vk::WholeSize,
        };
    }
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eVertexShader, {}, 0, nullptr,
        (uint32_t) vertex_buffer_upload_barriers.size(),
        vertex_buffer_upload_barriers.data(), 0, nullptr);
    // fragment barrier
    vk::BufferMemoryBarrier const fragment_material_buffer_barrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .buffer = rasterization.material_buffer.buffer,
        .offset = 0,
        .size = vk::WholeSize,
    };
    vk::BufferMemoryBarrier const fragment_light_buffer_barrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .buffer = rasterization.light_buffer.buffer,
        .offset = 0,
        .size = vk::WholeSize,
    };
    std::array const fragment_buffer_barriers{
        fragment_material_buffer_barrier, fragment_light_buffer_barrier};
    std::vector<vk::ImageMemoryBarrier> fragment_image_barriers{};
    for (uint32_t t = 0; t < rasterization.texture_array.size(); ++t) {
        fragment_image_barriers.push_back(vk::ImageMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead,
            .oldLayout = vk::ImageLayout::eGeneral,
            .newLayout = vk::ImageLayout::eGeneral,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = rasterization.texture_array[t].image,
            .subresourceRange = vk::ImageSubresourceRange{
                                                          .aspectMask = vk::ImageAspectFlagBits::eColor,
                                                          .baseMipLevel = 0,
                                                          .levelCount = rasterization.texture_array[t].level,
                                                          .baseArrayLayer = 0,
                                                          .layerCount = rasterization.texture_array[t].layer}
        });
    }
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr,
        (uint32_t) fragment_buffer_barriers.size(),
        fragment_buffer_barriers.data(),
        (uint32_t) fragment_image_barriers.size(),
        fragment_image_barriers.data());
    for (uint32_t f = 0; f < FRAME_IN_FLIGHT; ++f) {
        // set 0
        update_descriptor_storage_buffer_whole(device,
            rasterization.descriptor_sets[0][f], 0, 0,
            rasterization.vertices_buffer);
        update_descriptor_storage_buffer_whole(device,
            rasterization.descriptor_sets[0][f], 1, 0,
            rasterization.transformation_buffer);
        update_descriptor_storage_buffer_whole(device,
            rasterization.descriptor_sets[0][f], 2, 0,
            rasterization.instance_buffer);
        // set 1
        update_descriptor_storage_buffer_whole(device,
            rasterization.descriptor_sets[1][f], 0, 0,
            rasterization.material_buffer);
        update_descriptor_storage_buffer_whole(device,
            rasterization.descriptor_sets[1][f], 1, 0,
            rasterization.light_buffer);
        for (uint32_t t = 0; t < rasterization.texture_array.size(); ++t) {
            update_descriptor_image_sampler_combined(device,
                rasterization.descriptor_sets[1][f], 2, t, primary_sampler,
                rasterization.texture_array[t].primary_view);
        }
    }
}

static void clean_rasterization_resources() {
    destroy_buffer(vma_alloc, rasterization.indirect_draw_buffer);
    destroy_buffer(vma_alloc, rasterization.vertices_buffer);
    destroy_buffer(vma_alloc, rasterization.transformation_buffer);
    destroy_buffer(vma_alloc, rasterization.instance_buffer);
    destroy_buffer(vma_alloc, rasterization.material_buffer);
    destroy_buffer(vma_alloc, rasterization.light_buffer);
    for (uint32_t i = 0; i < rasterization.texture_array.size(); ++i) {
        destroy_image(device, vma_alloc, rasterization.texture_array[i]);
    }
    rasterization.texture_array.clear();
}

void load_rasterizer(renderer& renderer) {
    renderer.initialize = rasterizer_initialize;
    renderer.prepare_data = rasterizer_prepare_data;
    renderer.update_data = rasterizer_update_data;
    renderer.render = rasterizer_render;
    renderer.present = rasterizer_present;
    renderer.destroy = rasterizer_destroy;
}

void rasterizer_initialize(render_options const&) {
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
        VK_CHECK_CREATE(result, graphics_semaphores[f],
            device.createSemaphore(semaphore_info));
        VK_CHECK_CREATE(result, present_semaphores[f],
            device.createSemaphore(semaphore_info));
    }
    create_frame_objects();
    create_rasterization_pipeline();
}

void rasterizer_prepare_data(scene const& scene) {
    prepare_rasterization_resources(scene);
}

void rasterizer_update_data(scene const&) {
}

void rasterizer_render(camera const& camera) {
    vk::Result result;
    auto const [graphics_command_buffer, sync_idx] =
        get_command_buffer(vk::PipelineBindPoint::eGraphics);
    result = swapchain_acquire_next_image_wrapper(device,
        frame_objects.swapchain, 1e9, present_semaphores[sync_idx], nullptr,
        &frame_objects.swapchain_image_idx);
    if (result == vk::Result::eErrorOutOfDateKHR) {
        refresh_frame_objects();
        return;
    }
    CHECK(
        result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR,
        "");
    vk::Rect2D const render_area{
        .offset = {0, 0},
        .extent = swapchain_extent,
    };
    std::array const clear_values{
        vk::ClearValue{.color = {std::array{0.0f, 0.0f, 0.0f, 1.0f}}},
        vk::ClearValue{.depthStencil = {1.0f, 0}},
    };
    vk::RenderPassBeginInfo const render_pass_begin_info{
        .renderPass = frame_objects.render_pass,
        .framebuffer =
            frame_objects.framebuffers[frame_objects.swapchain_image_idx],
        .renderArea = render_area,
        .clearValueCount = (uint32_t) clear_values.size(),
        .pClearValues = clear_values.data(),
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
    // rasterization
    std::array<vk::DescriptorSet, RASTERIZATION_SET> disney_brdf_sets{};
    for (uint32_t s = 0; s < RASTERIZATION_SET; ++s) {
        disney_brdf_sets[s] = rasterization.descriptor_sets[s][sync_idx];
    }
    graphics_command_buffer.bindPipeline(
        vk::PipelineBindPoint::eGraphics, rasterization.pipeline);
    graphics_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        rasterization.pipeline_layout, 0, (uint32_t) disney_brdf_sets.size(),
        disney_brdf_sets.data(), 0, nullptr);
    glm::mat4 const proj_view = get_glsl_rasterizer_camera(
        camera, swapchain_extent.width, swapchain_extent.height);
    rasterization_vert_pc const disney_brdf_vert_pc{
        .proj_view = proj_view,
    };
    graphics_command_buffer.pushConstants(rasterization.pipeline_layout,
        vk::ShaderStageFlagBits::eVertex, 0,
        (uint32_t) sizeof(disney_brdf_vert_pc), &disney_brdf_vert_pc);
    graphics_command_buffer.drawIndirect(
        rasterization.indirect_draw_buffer.buffer, 0, mesh_instance_count,
        (uint32_t) sizeof(vk::DrawIndirectCommand));
    graphics_command_buffer.endRenderPass();
    add_submit_wait(vk::PipelineBindPoint::eGraphics,
        present_semaphores[sync_idx],
        vk::PipelineStageFlagBits::eColorAttachmentOutput);
    add_submit_signal(
        vk::PipelineBindPoint::eGraphics, graphics_semaphores[sync_idx]);
    add_present_wait(graphics_semaphores[sync_idx]);
}

void rasterizer_present() {
    vk::Result const result =
        present(frame_objects.swapchain, frame_objects.swapchain_image_idx);
    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
        refresh_frame_objects();
    } else {
        CHECK(result == vk::Result::eSuccess, "");
    }
}

void rasterizer_destroy() {
    if (!initialized) {
        return;
    }
    initialized = false;
    device.destroyDescriptorPool(primary_descriptor_pool);
    device.destroySampler(primary_sampler);
    for (uint32_t f = 0; f < FRAME_IN_FLIGHT; ++f) {
        device.destroySemaphore(graphics_semaphores[f]);
        device.destroySemaphore(present_semaphores[f]);
    }
    cleanup_staging_buffer(vma_alloc);
    cleanup_staging_image(vma_alloc);
    clean_rasterization_resources();
    destroy_rasterization_pipeline();
    destroy_frame_objects();
}
