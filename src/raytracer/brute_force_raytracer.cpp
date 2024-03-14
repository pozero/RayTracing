#include "raytracer/raytracer.h"

void brute_force_raytracer() {
    //////////////////////////////
    ///* Initialization: GLFW *///
    //////////////////////////////
    auto const [win_width, win_height] = get_frame_size1();
    GLFWwindow* window = glfw_create_window((int) win_width, (int) win_height);
    //////////////////////////////
    ///* Initialization: GLFW *///
    //////////////////////////////

    ////////////////////////////
    ///* Prepare Scene Data *///
    ////////////////////////////
    auto [camera, spheres, triangle_mesh, texture_datas] =
        get_scene1(win_width, win_height);
    glsl_sky_color sky_color = get_sky_color1();
    ////////////////////////////
    ///* Prepare Scene Data *///
    ////////////////////////////

    //////////////////////////////////
    ///* Initialization: Instance *///
    //////////////////////////////////
    vk::Result result;
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
    vk::Instance instance = create_instance(inst_ext, inst_layer);
    //////////////////////////////////
    ///* Initialization: Instance *///
    //////////////////////////////////

#if defined(VK_DBG)
    vk::DebugUtilsMessengerEXT dbg_messenger = create_debug_messenger(instance);
#endif

    /////////////////////////////////
    ///* Initialization: Surface *///
    /////////////////////////////////
    vk::SurfaceKHR surface = create_surface(instance, window);
    /////////////////////////////////
    ///* Initialization: Surface *///
    /////////////////////////////////

    ////////////////////////////////
    ///* Initialization: Device *///
    ////////////////////////////////
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
    vk::PhysicalDeviceFeatures const physical_device_features{};
    auto [dev, phy_dev, queues] =
        select_physical_device_create_device_queues(instance, surface, dev_ext,
            dev_creation_pnext, physical_device_features);
    VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptor_indexing_properties{
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT,
    };
    VkPhysicalDeviceProperties2KHR phy_dev_properties{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR,
        .pNext = &descriptor_indexing_properties,
    };
    vk::defaultDispatchLoaderDynamic.vkGetPhysicalDeviceProperties2(
        phy_dev, &phy_dev_properties);
    fmt::println(
        "Selected device: {}", phy_dev.getProperties().deviceName.data());
    ////////////////////////////////
    ///* Initialization: Device *///
    ////////////////////////////////

    /////////////////////////////
    ///* Initialization: VMA *///
    /////////////////////////////
    VmaAllocator vma_alloc = create_vma_allocator(instance, phy_dev, dev);
    /////////////////////////////
    ///* Initialization: VMA *///
    /////////////////////////////

    ///////////////////////////////////
    ///* Initialization: Swapchain *///
    ///////////////////////////////////
    prepare_swapchain(phy_dev, surface);
    wait_window(dev, phy_dev, surface, window);
    auto [swapchain, swapchain_images, swapchain_image_views] =
        create_swapchain(dev, surface, queues);
    ///////////////////////////////////
    ///* Initialization: Swapchain *///
    ///////////////////////////////////

    /////////////////////////////////////////////////////
    ///* Initialization: Render Pass And Framebuffer *///
    /////////////////////////////////////////////////////
    vk::RenderPass render_pass = create_render_pass(dev);
    std::vector<vk::Framebuffer> framebuffers =
        create_framebuffers(dev, render_pass, swapchain_image_views);
    /////////////////////////////////////////////////////
    ///* Initialization: Render Pass And Framebuffer *///
    /////////////////////////////////////////////////////

    size_t constexpr FRAME_IN_FLIGHT = 3;

    auto const recreate_swapchain = [&]() {
        wait_window(dev, phy_dev, surface, window);
        for (auto const framebuffer : framebuffers) {
            dev.destroyFramebuffer(framebuffer);
        }
        for (auto const image_view : swapchain_image_views) {
            dev.destroyImageView(image_view);
        }
        swapchain_images.clear();
        dev.destroySwapchainKHR(swapchain);
        std::tie(swapchain, swapchain_images, swapchain_image_views) =
            create_swapchain(dev, surface, queues);
        framebuffers =
            create_framebuffers(dev, render_pass, swapchain_image_views);
    };

    /////////////////////////////////////////
    ///* Initialization: Command Buffers *///
    /////////////////////////////////////////
    auto [graphics_command_pool, graphics_command_buffers] =
        create_command_buffer(dev, queues.graphics_queue_idx, FRAME_IN_FLIGHT);
    auto [raytracing_command_pool, raytracing_command_buffers] =
        create_command_buffer(dev, queues.compute_queue_idx, FRAME_IN_FLIGHT);
    /////////////////////////////////////////
    ///* Initialization: Command Buffers *///
    /////////////////////////////////////////

    /////////////////////////////////////
    ///* Initialization: Descriptors *///
    /////////////////////////////////////
    vk::DescriptorPool default_descriptor_pool = create_descriptor_pool(dev);
    vk::DescriptorPool descriptor_indexing_pool = create_descriptor_pool(
        dev, vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind);
    // graphics pipeline
    std::vector<vk_descriptor_set_binding> const
        graphics_pipeline_set_0_binding{
            {vk::DescriptorType::eCombinedImageSampler, 1},
            {       vk::DescriptorType::eUniformBuffer, 1},
    };
    vk::DescriptorSetLayout graphics_pipeline_set_0_layout =
        create_descriptor_set_layout(dev, vk::ShaderStageFlagBits::eFragment,
            graphics_pipeline_set_0_binding);
    vk::PipelineLayout graphics_pipeline_layout =
        create_pipeline_layout(dev, {graphics_pipeline_set_0_layout});
    std::vector<vk::DescriptorSet> graphics_pipeline_set_0s =
        create_descriptor_set(dev, default_descriptor_pool,
            graphics_pipeline_set_0_layout, FRAME_IN_FLIGHT);
    // raytracing pipeline
    std::vector<vk_descriptor_set_binding> const
        raytracing_pipeline_set_0_binding{
            { vk::DescriptorType::eStorageImage, 1},
            {vk::DescriptorType::eUniformBuffer, 1},
            {vk::DescriptorType::eUniformBuffer, 1},
            {vk::DescriptorType::eUniformBuffer, 1},
            {vk::DescriptorType::eUniformBuffer, 1},
            {vk::DescriptorType::eUniformBuffer, 1},
    };
    std::vector<vk_descriptor_set_binding> const
        raytracing_pipeline_set_1_binding{
            {
             vk::DescriptorType::eCombinedImageSampler,
             descriptor_indexing_properties
             .maxDescriptorSetUpdateAfterBindSampledImages,
             vk::DescriptorBindingFlagBits::eVariableDescriptorCount |
             vk::DescriptorBindingFlagBits::eUpdateAfterBind |
             vk::DescriptorBindingFlagBits::ePartiallyBound |
             vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending,
             },
    };
    struct raytracing_pipeline_push_constants {
        glm::vec3 sky_color_top;
        glm::vec3 sky_color_bottom;
        uint32_t frame_counter;
    };
    vk::DescriptorSetLayout raytracing_pipeline_set_0_layout =
        create_descriptor_set_layout(dev, vk::ShaderStageFlagBits::eCompute,
            raytracing_pipeline_set_0_binding);
    vk::DescriptorSetLayout raytracing_pipeline_set_1_layout =
        create_descriptor_set_layout(dev, vk::ShaderStageFlagBits::eCompute,
            raytracing_pipeline_set_1_binding,
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);
    vk::PipelineLayout raytracing_pipeline_layout = create_pipeline_layout(dev,
        {(uint32_t) sizeof(raytracing_pipeline_push_constants)},
        {vk::ShaderStageFlagBits::eCompute},
        {raytracing_pipeline_set_0_layout, raytracing_pipeline_set_1_layout});
    std::vector<vk::DescriptorSet> raytracing_pipeline_set_0s =
        create_descriptor_set(dev, default_descriptor_pool,
            raytracing_pipeline_set_0_layout, FRAME_IN_FLIGHT);
    std::vector<vk::DescriptorSet> raytracing_pipeline_set_1s =
        create_descriptor_set(dev, descriptor_indexing_pool,
            raytracing_pipeline_set_1_layout, FRAME_IN_FLIGHT,
            (uint32_t) texture_datas.size());
    /////////////////////////////////////
    ///* Initialization: Descriptors *///
    /////////////////////////////////////

    //////////////////////////////////
    ///* Initialization: Pipeline *///
    //////////////////////////////////
    vk::Pipeline graphics_pipeline =
        create_graphics_pipeline(dev, PATH_FROM_BINARY("shaders/rect.vert.spv"),
            PATH_FROM_BINARY("shaders/rect.frag.spv"), graphics_pipeline_layout,
            render_pass, {}, vk::PolygonMode::eFill, false);
    vk::Pipeline raytracing_pipeline = create_compute_pipeline(dev,
        PATH_FROM_BINARY("shaders/raytracer.comp.spv"),
        raytracing_pipeline_layout,
        {(uint32_t) spheres.size(), (uint32_t) triangle_mesh.vertices.size(),
            (uint32_t) triangle_mesh.triangles.size(),
            (uint32_t) triangle_mesh.materials.size()});
    //////////////////////////////////
    ///* Initialization: Pipeline *///
    //////////////////////////////////

    ///////////////////////////////////////////
    ///* Initialization: Frame Sync Object *///
    ///////////////////////////////////////////
    uint32_t frame_counter = 0;
    vk::FenceCreateInfo const fence_info{
        .flags = vk::FenceCreateFlagBits::eSignaled,
    };
    vk::SemaphoreCreateInfo const semaphore_info{};
    std::vector<vk::Fence> render_fences(FRAME_IN_FLIGHT);
    std::vector<vk::Fence> raytracing_fences(FRAME_IN_FLIGHT);
    std::vector<vk::Semaphore> render_semaphores(FRAME_IN_FLIGHT);
    std::vector<vk::Semaphore> present_semaphores(FRAME_IN_FLIGHT);
    std::vector<vk::Semaphore> raytracing_semaphores(FRAME_IN_FLIGHT);
    for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {
        VK_CHECK_CREATE(result, render_fences[i], dev.createFence(fence_info));
        VK_CHECK_CREATE(
            result, raytracing_fences[i], dev.createFence(fence_info));
        VK_CHECK_CREATE(
            result, render_semaphores[i], dev.createSemaphore(semaphore_info));
        VK_CHECK_CREATE(
            result, present_semaphores[i], dev.createSemaphore(semaphore_info));
        VK_CHECK_CREATE(result, raytracing_semaphores[i],
            dev.createSemaphore(semaphore_info));
    }
    ///////////////////////////////////////////
    ///* Initialization: Frame Sync Object *///
    ///////////////////////////////////////////

    ///////////////////////////////////////
    ///* Initialization: Image, Buffer *///
    ///////////////////////////////////////
    create_dummy_buffer(vma_alloc);
    vk::Sampler default_sampler = create_default_sampler(dev);
    std::array<vma_image, FRAME_IN_FLIGHT> accumulation_images{};
    std::vector<vma_image> textures{};
    std::array<vma_buffer, FRAME_IN_FLIGHT> camera_buffers{};
    vma_buffer sphere_buffer{};
    vma_buffer triangle_vertex_buffer{};
    vma_buffer triangle_face_buffer{};
    vma_buffer triangle_material_buffer{};
    {
        ++frame_counter;
        vk::Fence const fence = render_fences[0];
        vk::CommandBuffer const command_buffer = graphics_command_buffers[0];
        VK_CHECK(result, command_buffer.reset());
        VK_CHECK(result, dev.resetFences(1, &fence));
        vk::CommandBufferBeginInfo const begin_info{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        };
        VK_CHECK(result, command_buffer.begin(begin_info));
        sphere_buffer = create_gpu_only_buffer(vma_alloc, size_in_byte(spheres),
            {}, vk::BufferUsageFlagBits::eUniformBuffer);
        triangle_vertex_buffer = create_gpu_only_buffer(vma_alloc,
            size_in_byte(triangle_mesh.vertices), {},
            vk::BufferUsageFlagBits::eUniformBuffer);
        triangle_face_buffer = create_gpu_only_buffer(vma_alloc,
            size_in_byte(triangle_mesh.triangles), {},
            vk::BufferUsageFlagBits::eUniformBuffer);
        triangle_material_buffer = create_gpu_only_buffer(vma_alloc,
            size_in_byte(triangle_mesh.materials), {},
            vk::BufferUsageFlagBits::eUniformBuffer);
        update_buffer(
            vma_alloc, command_buffer, sphere_buffer, to_span(spheres), 0);
        update_buffer(vma_alloc, command_buffer, triangle_vertex_buffer,
            to_span(triangle_mesh.vertices), 0);
        update_buffer(vma_alloc, command_buffer, triangle_face_buffer,
            to_span(triangle_mesh.triangles), 0);
        update_buffer(vma_alloc, command_buffer, triangle_material_buffer,
            to_span(triangle_mesh.materials), 0);
        // textures
        textures.reserve(texture_datas.size());
        for (uint32_t texture_idx = 0; texture_idx < texture_datas.size();
             ++texture_idx) {
            vk::Format const format =
                texture_datas[texture_idx].format == texture_format::unorm ?
                    vk::Format::eR8G8B8A8Unorm :
                    vk::Format::eR32G32B32A32Sfloat;
            textures.push_back(create_texture2d(dev, vma_alloc, command_buffer,
                texture_datas[texture_idx].width,
                texture_datas[texture_idx].height, 1, format, {},
                vk::ImageUsageFlagBits::eSampled |
                    vk::ImageUsageFlagBits::eTransferDst));
            update_texture2d(vma_alloc, textures[texture_idx], command_buffer,
                texture_datas[texture_idx]);
        }
        for (uint32_t frame_idx = 0; frame_idx < FRAME_IN_FLIGHT; ++frame_idx) {
            accumulation_images[frame_idx] =
                create_texture2d(dev, vma_alloc, command_buffer, win_width,
                    win_height, 1, vk::Format::eR32G32B32A32Sfloat,
                    std::vector<uint32_t>{
                        queues.compute_queue_idx,
                        queues.graphics_queue_idx,
                    },
                    vk::ImageUsageFlagBits::eStorage |
                        vk::ImageUsageFlagBits::eTransferDst |
                        vk::ImageUsageFlagBits::eTransferSrc);
            camera_buffers[frame_idx] = create_gpu_only_buffer(vma_alloc,
                (uint32_t) sizeof(glsl_raytracer_camera), {},
                vk::BufferUsageFlagBits::eUniformBuffer);
            // graphics pipeline set 0
            update_descriptor_uniform_buffer_whole(dev,
                graphics_pipeline_set_0s[frame_idx], 1, 0,
                camera_buffers[frame_idx]);
            // raytracing pipeline set 0
            update_descriptor_uniform_buffer_whole(dev,
                raytracing_pipeline_set_0s[frame_idx], 1, 0,
                camera_buffers[frame_idx]);
            update_descriptor_uniform_buffer_whole(dev,
                raytracing_pipeline_set_0s[frame_idx], 2, 0, sphere_buffer);
            update_descriptor_uniform_buffer_whole(dev,
                raytracing_pipeline_set_0s[frame_idx], 3, 0,
                triangle_vertex_buffer);
            update_descriptor_uniform_buffer_whole(dev,
                raytracing_pipeline_set_0s[frame_idx], 4, 0,
                triangle_face_buffer);
            update_descriptor_uniform_buffer_whole(dev,
                raytracing_pipeline_set_0s[frame_idx], 5, 0,
                triangle_material_buffer);
            // raytracing pipeline set 1
            for (uint32_t texture_idx = 0; texture_idx < texture_datas.size();
                 ++texture_idx) {
                update_descriptor_image_sampler_combined(dev,
                    raytracing_pipeline_set_1s[frame_idx], 0, texture_idx,
                    default_sampler, textures[texture_idx].primary_view);
            }
        }
        VK_CHECK(result, command_buffer.end());
        vk::SubmitInfo const submit_info{
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };
        VK_CHECK(result, queues.graphics_queue.submit(1, &submit_info, fence));
    }
    ///////////////////////////////////////
    ///* Initialization: Image, Buffer *///
    ///////////////////////////////////////

    high_resolution_clock clock{};
    clock.tick();

    ////////////////////
    ///* RenderLoop *///
    ////////////////////
    while (!glfwWindowShouldClose(window)) {
        clock.tick();

        ///////////////////////
        ///* Process Input *///
        ///////////////////////
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, 1);
        }
        update_camera(window, camera, clock.get_delta_seconds());
        ///////////////////////
        ///* Process Input *///
        ///////////////////////

        uint32_t const frame_sync_idx = frame_counter % FRAME_IN_FLIGHT;
        vk::Fence const render_fence = render_fences[frame_sync_idx];
        vk::Fence const raytracing_fence = raytracing_fences[frame_sync_idx];
        vk::Semaphore const render_semaphore =
            render_semaphores[frame_sync_idx];
        vk::Semaphore const present_semaphore =
            present_semaphores[frame_sync_idx];
        vk::Semaphore const raytracing_semaphore =
            raytracing_semaphores[frame_sync_idx];
        vk::CommandBuffer const graphics_command_buffer =
            graphics_command_buffers[frame_sync_idx];
        vk::CommandBuffer const raytracing_command_buffer =
            raytracing_command_buffers[frame_sync_idx];
        vk::CommandBufferBeginInfo const begin_info{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        };
        std::array const graphics_pipeline_sets{
            graphics_pipeline_set_0s[frame_sync_idx],
        };
        std::array const raytracing_pipieline_sets{
            raytracing_pipeline_set_0s[frame_sync_idx],
            raytracing_pipeline_set_1s[frame_sync_idx],
        };
        vma_buffer camera_buffer = camera_buffers[frame_sync_idx];
        vma_image accumulation_image;
        raytracing_pipeline_push_constants const
            raytracing_pipeline_push_constants{
                .sky_color_top = sky_color.sky_color_top,
                .sky_color_bottom = sky_color.sky_color_bottom,
                .frame_counter = frame_counter,
            };

        /////////////////////////////
        ///* Raytracing Pipeline *///
        /////////////////////////////
        VK_CHECK(result, dev.waitForFences(1, &raytracing_fence, true, 1e9));
        VK_CHECK(result, dev.resetFences(1, &raytracing_fence));
        VK_CHECK(result, raytracing_command_buffer.reset());
        VK_CHECK(result, raytracing_command_buffer.begin(begin_info));
        if (camera.dirty) {
            camera.dirty = false;
            camera.frame_counter = 1;
            camera.accumulation_idx =
                (camera.accumulation_idx + 1) % accumulation_images.size();
            accumulation_image = accumulation_images[camera.accumulation_idx];
            std::array const black{0.0f, 0.0f, 0.0f, 1.0f};
            vk::ImageSubresourceRange const whole_range{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };
            vk::ClearColorValue accumulation_clear_value{.float32 = black};
            raytracing_command_buffer.clearColorImage(accumulation_image.image,
                vk::ImageLayout::eGeneral, &accumulation_clear_value, 1,
                &whole_range);
            vk::ImageMemoryBarrier const barrier{
                .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
                .dstAccessMask = vk::AccessFlagBits::eShaderRead |
                                 vk::AccessFlagBits::eShaderWrite,
                .oldLayout = vk::ImageLayout::eGeneral,
                .newLayout = vk::ImageLayout::eGeneral,
                .srcQueueFamilyIndex = queues.compute_queue_idx,
                .dstQueueFamilyIndex = queues.compute_queue_idx,
                .image = accumulation_image.image,
                .subresourceRange = whole_range,
            };
            raytracing_command_buffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0,
                nullptr, 1, &barrier);
        } else {
            accumulation_image = accumulation_images[camera.accumulation_idx];
        }
        glsl_raytracer_camera const glsl_camera =
            get_glsl_raytracer_camera(camera, win_width, win_height);
        update_buffer(vma_alloc, raytracing_command_buffer, camera_buffer,
            to_span(glsl_camera), 0);
        vk::BufferMemoryBarrier const camera_buffer_barrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead,
            .srcQueueFamilyIndex = queues.compute_queue_idx,
            .dstQueueFamilyIndex = queues.compute_queue_idx,
            .buffer = camera_buffer.buffer,
            .offset = 0,
            .size = vk::WholeSize,
        };
        raytracing_command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 1,
            &camera_buffer_barrier, 0, nullptr);
        update_descriptor_storage_image(dev, raytracing_pipieline_sets[0], 0, 0,
            accumulation_image.primary_view);
        raytracing_command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute, raytracing_pipeline_layout, 0,
            (uint32_t) raytracing_pipieline_sets.size(),
            raytracing_pipieline_sets.data(), 0, nullptr);
        raytracing_command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute, raytracing_pipeline);
        raytracing_command_buffer.pushConstants(raytracing_pipeline_layout,
            vk::ShaderStageFlagBits::eCompute, 0,
            sizeof(raytracing_pipeline_push_constants),
            &raytracing_pipeline_push_constants);
        raytracing_command_buffer.dispatch(win_width, win_height, 1);
        VK_CHECK(result, raytracing_command_buffer.end());
        std::array raytracing_pipeline_signal_semaphores{
            raytracing_semaphore,
        };
        vk::SubmitInfo const raytracing_submit_info{
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &raytracing_command_buffer,
            .signalSemaphoreCount =
                (uint32_t) raytracing_pipeline_signal_semaphores.size(),
            .pSignalSemaphores = raytracing_pipeline_signal_semaphores.data(),
        };
        VK_CHECK(result, queues.compute_queue.submit(
                             1, &raytracing_submit_info, raytracing_fence));
        /////////////////////////////
        ///* Raytracing Pipeline *///
        /////////////////////////////

        ///////////////////////////
        ///* Graphics Pipeline *///
        ///////////////////////////
        VK_CHECK(result, dev.waitForFences(1, &render_fence, true, 1e9));
        uint32_t swapchain_image_idx = 0;
        result = swapchain_acquire_next_image_wrapper(dev, swapchain, 1e9,
            present_semaphore, nullptr, &swapchain_image_idx);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreate_swapchain();
            continue;
        }
        CHECK(result == vk::Result::eSuccess ||
                  result == vk::Result::eSuboptimalKHR,
            "");
        VK_CHECK(result, dev.resetFences(1, &render_fence));
        VK_CHECK(result, graphics_command_buffer.reset());
        VK_CHECK(result, graphics_command_buffer.begin(begin_info));
        vk::Rect2D const render_area{
            .offset = {0, 0},
            .extent = swapchain_extent,
        };
        vk::ClearValue const color_clear_val{
            .color = {std::array{0.243f, 0.706f, 0.537f, 1.0f}},
        };
        vk::RenderPassBeginInfo const render_pass_begin_info{
            .renderPass = render_pass,
            .framebuffer = framebuffers[swapchain_image_idx],
            .renderArea = render_area,
            .clearValueCount = 1,
            .pClearValues = &color_clear_val,
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
        update_descriptor_image_sampler_combined(dev, graphics_pipeline_sets[0],
            0, 0, default_sampler, accumulation_image.primary_view);
        graphics_command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, graphics_pipeline_layout, 0,
            (uint32_t) graphics_pipeline_sets.size(),
            graphics_pipeline_sets.data(), 0, nullptr);
        graphics_command_buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics, graphics_pipeline);
        graphics_command_buffer.draw(3, 1, 0, 0);
        graphics_command_buffer.endRenderPass();
        VK_CHECK(result, graphics_command_buffer.end());
        std::array const graphics_submit_wait_semaphores{
            present_semaphore,
            raytracing_semaphore,
        };
        std::array<vk::PipelineStageFlags, 2> const graphics_submit_wait_stages{
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eComputeShader,
        };
        vk::SubmitInfo const graphics_submit_info{
            .waitSemaphoreCount =
                (uint32_t) graphics_submit_wait_semaphores.size(),
            .pWaitSemaphores = graphics_submit_wait_semaphores.data(),
            .pWaitDstStageMask = graphics_submit_wait_stages.data(),
            .commandBufferCount = 1,
            .pCommandBuffers = &graphics_command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_semaphore,
        };
        VK_CHECK(result, queues.graphics_queue.submit(
                             1, &graphics_submit_info, render_fence));
        vk::PresentInfoKHR const present_info{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &swapchain_image_idx,
        };
        result = swapchain_present_wrapper(queues.present_queue, present_info);
        if (result == vk::Result::eErrorOutOfDateKHR ||
            result == vk::Result::eSuboptimalKHR) {
            recreate_swapchain();
        } else {
            CHECK(result == vk::Result::eSuccess, "");
        }
        ///////////////////////////
        ///* Graphics Pipeline *///
        ///////////////////////////

        glfwPollEvents();
        ++frame_counter;
        ++camera.frame_counter;
    }
    ////////////////////
    ///* RenderLoop *///
    ////////////////////

    /////////////////
    ///* Cleanup *///
    /////////////////
    for (auto& texture_data : texture_datas) {
        std::free(texture_data.data);
    }
    glfwTerminate();
    VK_CHECK(result, dev.waitIdle());
    destroy_buffer(vma_alloc, sphere_buffer);
    destroy_buffer(vma_alloc, triangle_vertex_buffer);
    destroy_buffer(vma_alloc, triangle_face_buffer);
    destroy_buffer(vma_alloc, triangle_material_buffer);
    for (auto const& t : textures) {
        destroy_image(dev, vma_alloc, t);
    }
    for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {
        destroy_image(dev, vma_alloc, accumulation_images[i]);
        destroy_buffer(vma_alloc, camera_buffers[i]);
    }
    cleanup_staging_buffer(vma_alloc);
    cleanup_staging_image(vma_alloc);
    destroy_dummy_buffer(vma_alloc);
    dev.destroySampler(default_sampler);
    for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {
        dev.destroyFence(render_fences[i]);
        dev.destroyFence(raytracing_fences[i]);
        dev.destroySemaphore(render_semaphores[i]);
        dev.destroySemaphore(present_semaphores[i]);
        dev.destroySemaphore(raytracing_semaphores[i]);
    }
    dev.destroyPipeline(graphics_pipeline);
    dev.destroyPipeline(raytracing_pipeline);
    dev.destroyPipelineLayout(graphics_pipeline_layout);
    dev.destroyPipelineLayout(raytracing_pipeline_layout);
    dev.destroyDescriptorSetLayout(graphics_pipeline_set_0_layout);
    dev.destroyDescriptorSetLayout(raytracing_pipeline_set_0_layout);
    dev.destroyDescriptorSetLayout(raytracing_pipeline_set_1_layout);
    dev.destroyDescriptorPool(descriptor_indexing_pool);
    dev.destroyDescriptorPool(default_descriptor_pool);
    dev.destroyCommandPool(graphics_command_pool);
    dev.destroyCommandPool(raytracing_command_pool);
    for (auto const framebuffer : framebuffers) {
        dev.destroyFramebuffer(framebuffer);
    }
    dev.destroyRenderPass(render_pass);
    for (auto const view : swapchain_image_views) {
        dev.destroyImageView(view);
    }
    dev.destroySwapchainKHR(swapchain);
    vmaDestroyAllocator(vma_alloc);
    dev.destroy();
    instance.destroySurfaceKHR(surface);
#if defined(VK_DBG)
    instance.destroyDebugUtilsMessengerEXT(dbg_messenger);
#endif
    instance.destroy();
    /////////////////
    ///* Cleanup *///
    /////////////////
}
