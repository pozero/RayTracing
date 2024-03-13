#include "renderer/renderer.h"

static uint32_t win_width = 1280;
static uint32_t win_height = 720;

void cook_torrance_brdf_renderer() {
    //////////////////////////////
    ///* Initialization: GLFW *///
    //////////////////////////////
    auto const glfw_resize_callback = [](GLFWwindow* window, int width,
                                          int height) {
        (void) window;
        win_width = (uint32_t) width;
        win_height = (uint32_t) height;
    };
    GLFWwindow* window = glfw_create_window((int) win_width, (int) win_height);
    glfwSetFramebufferSizeCallback(window, glfw_resize_callback);
    //////////////////////////////
    ///* Initialization: GLFW *///
    //////////////////////////////

    ////////////////////////////
    ///* Prepare Scene Data *///
    ////////////////////////////
    camera camera = create_camera(glm::vec3{0.0f, 0.0f, 5.0f},
        glm::vec3{0.0f, 0.0f, 0.0f}, 45.0f, win_width, win_height);
    triangle_mesh triangle_mesh{};
    {
        uint32_t constexpr SPHERE_ROWS = 7;
        uint32_t constexpr SPHERE_COLUMNS = 7;
        glm::vec4 const albedo{0.5f, 0.0f, 0.0f, 1.0f};
        float const ao = 1.0f;
        float constexpr SPACING = 2.5f;
        float constexpr RADIUS = 1.0f;
        for (uint32_t row = 0; row < SPHERE_ROWS; ++row) {
            float const metallic = (float) row / (float) SPHERE_ROWS;
            for (uint32_t col = 0; col < SPHERE_COLUMNS; ++col) {
                float const roughness = std::clamp(
                    ((float) col / (float) SPHERE_COLUMNS), 0.05f, 1.0f);
                cook_torrance_material const material{
                    .albedo = albedo,
                    .metallic = metallic,
                    .roughness = roughness,
                    .ao = ao,
                };
                glm::vec3 const center{
                    ((float) col - ((float) SPHERE_COLUMNS * 0.5f)) * SPACING,
                    ((float) row - ((float) SPHERE_ROWS * 0.5f)) * SPACING,
                    0.0f};
                glsl_sphere const sphere{
                    .center = center,
                    .radius = RADIUS,
                };
                triangulate_sphere(triangle_mesh, sphere, material, 64);
            }
        }
    }
    std::vector<point_light> const point_lights{
        point_light{.position = glm::vec4(-10.0f,  10.0f, 10.0f, 1.0f),
                    .color = glm::vec4(300.0f, 300.0f, 300.0f, 1.0f)},
        point_light{ .position = glm::vec4(10.0f,  10.0f, 10.0f, 1.0f),
                    .color = glm::vec4(300.0f, 300.0f, 300.0f, 1.0f)},
        point_light{.position = glm::vec4(-10.0f, -10.0f, 10.0f, 1.0f),
                    .color = glm::vec4(300.0f, 300.0f, 300.0f, 1.0f)},
        point_light{ .position = glm::vec4(10.0f, -10.0f, 10.0f, 1.0f),
                    .color = glm::vec4(300.0f, 300.0f, 300.0f, 1.0f)}
    };
    texture_data background_hdr =
        get_texture_data(PATH_FROM_ROOT("assets/textures/computer_room.hdr"));
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
    vk::PhysicalDeviceFeatures const physical_device_deature{
        .sampleRateShading = vk::True,
        .fillModeNonSolid = vk::True,
    };
    auto [dev, phy_dev, queues] =
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
        phy_dev, &phy_dev_properties);
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
    auto [swapchain, swapchain_images, swapchain_image_views, depth_buffer,
        color_buffer] = create_swapchain_with_depth_multisampling(dev,
        vma_alloc, surface, queues);
    ///////////////////////////////////
    ///* Initialization: Swapchain *///
    ///////////////////////////////////

    /////////////////////////////////////////////////////
    ///* Initialization: Render Pass And Framebuffer *///
    /////////////////////////////////////////////////////
    vk::RenderPass render_pass =
        create_render_pass_with_depth_multisampling(dev);
    std::vector<vk::Framebuffer> framebuffers =
        create_framebuffers_with_depth_multisampling(dev, render_pass,
            swapchain_image_views, depth_buffer.primary_view,
            color_buffer.primary_view);
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
        destroy_image(dev, vma_alloc, depth_buffer);
        destroy_image(dev, vma_alloc, color_buffer);
        swapchain_images.clear();
        dev.destroySwapchainKHR(swapchain);
        std::tie(swapchain, swapchain_images, swapchain_image_views,
            depth_buffer, color_buffer) =
            create_swapchain_with_depth_multisampling(
                dev, vma_alloc, surface, queues);
        framebuffers = create_framebuffers_with_depth_multisampling(dev,
            render_pass, swapchain_image_views, depth_buffer.primary_view,
            color_buffer.primary_view);
    };

    /////////////////////////////////////////
    ///* Initialization: Command Buffers *///
    /////////////////////////////////////////
    auto [graphics_command_pool, graphics_command_buffers] =
        create_command_buffer(dev, queues.graphics_queue_idx, FRAME_IN_FLIGHT);
    auto [compute_command_pool, compute_command_buffers] =
        create_command_buffer(dev, queues.compute_queue_idx, FRAME_IN_FLIGHT);
    /////////////////////////////////////////
    ///* Initialization: Command Buffers *///
    /////////////////////////////////////////

    /////////////////////////////////////
    ///* Initialization: Descriptors *///
    /////////////////////////////////////
    vk::DescriptorPool default_descriptor_pool = create_descriptor_pool(dev);
    // generate cubemap pipeline
    std::vector<vk_descriptor_set_binding> const
        generate_cubemap_pipeline_set_0_binding{
            {vk::DescriptorType::eCombinedImageSampler, 1},
            {        vk::DescriptorType::eStorageImage, 1},
    };
    vk::DescriptorSetLayout generate_cubemap_pipeline_set_0_layout =
        create_descriptor_set_layout(dev, vk::ShaderStageFlagBits::eCompute,
            generate_cubemap_pipeline_set_0_binding);
    vk::PipelineLayout generate_cubemap_pipeline_layout =
        create_pipeline_layout(dev, {generate_cubemap_pipeline_set_0_layout});
    std::vector<vk::DescriptorSet> generate_cubemap_pipeline_set_0 =
        create_descriptor_set(dev, default_descriptor_pool,
            generate_cubemap_pipeline_set_0_layout, 1);
    // primary render pipeline
    struct primary_render_pipeline_vertex_push_constant {
        glm::mat4 proj_view;
    };
    struct primary_render_pipeline_fragment_push_constant {
        glm::vec4 camera_position;
    };
    std::vector<vk_descriptor_set_binding> const
        primary_render_pipeline_set_0_binding{
            {vk::DescriptorType::eStorageBuffer, 1},
            {vk::DescriptorType::eStorageBuffer, 1},
            {vk::DescriptorType::eStorageBuffer, 1},
    };
    std::vector<vk_descriptor_set_binding> const
        primary_render_pipeline_set_1_binding{
            {vk::DescriptorType::eStorageBuffer, 1},
            {vk::DescriptorType::eStorageBuffer, 1},
    };
    vk::DescriptorSetLayout primary_render_pipeline_set_0_layout =
        create_descriptor_set_layout(dev, vk::ShaderStageFlagBits::eVertex,
            primary_render_pipeline_set_0_binding);
    vk::DescriptorSetLayout primary_render_pipeline_set_1_layout =
        create_descriptor_set_layout(dev, vk::ShaderStageFlagBits::eFragment,
            primary_render_pipeline_set_1_binding);
    vk::PipelineLayout primary_render_pipeline_layout = create_pipeline_layout(
        dev,
        {(uint32_t) sizeof(primary_render_pipeline_vertex_push_constant),
            (uint32_t) sizeof(primary_render_pipeline_fragment_push_constant)},
        {vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment},
        {primary_render_pipeline_set_0_layout,
            primary_render_pipeline_set_1_layout});
    std::vector<vk::DescriptorSet> primary_render_pipeline_set_0s =
        create_descriptor_set(dev, default_descriptor_pool,
            primary_render_pipeline_set_0_layout, FRAME_IN_FLIGHT);
    std::vector<vk::DescriptorSet> primary_render_pipeline_set_1s =
        create_descriptor_set(dev, default_descriptor_pool,
            primary_render_pipeline_set_1_layout, FRAME_IN_FLIGHT);
    // environment render pipeline
    struct environment_render_pipeline_vertex_push_constant {
        glm::mat4 proj_view_for_environment_map;
    };
    std::vector<vk_descriptor_set_binding> const
        environment_render_pipeline_set_0_binding{
            {vk::DescriptorType::eCombinedImageSampler, 1},
    };
    vk::DescriptorSetLayout environment_render_pipeline_set_0_layout =
        create_descriptor_set_layout(dev, vk::ShaderStageFlagBits::eFragment,
            environment_render_pipeline_set_0_binding);
    vk::PipelineLayout environment_render_pipeline_layout =
        create_pipeline_layout(dev,
            {(uint32_t) sizeof(
                environment_render_pipeline_vertex_push_constant)},
            {vk::ShaderStageFlagBits::eVertex},
            {environment_render_pipeline_set_0_layout});
    std::vector<vk::DescriptorSet> environment_render_pipeline_set_0s =
        create_descriptor_set(dev, default_descriptor_pool,
            environment_render_pipeline_set_0_layout, FRAME_IN_FLIGHT);
    /////////////////////////////////////
    ///* Initialization: Descriptors *///
    /////////////////////////////////////

    //////////////////////////////////
    ///* Initialization: Pipeline *///
    //////////////////////////////////
    vk::Pipeline generate_cubemap_pipeline = create_compute_pipeline(dev,
        PATH_FROM_BINARY("shaders/cubemap.comp.spv"),
        generate_cubemap_pipeline_layout, {});
    vk::Pipeline primary_render_pipeline = create_graphics_pipeline(dev,
        PATH_FROM_BINARY("shaders/cook_torrance.vert.spv"),
        PATH_FROM_BINARY("shaders/cook_torrance.frag.spv"),
        primary_render_pipeline_layout, render_pass,
        {(uint32_t) point_lights.size()}, vk::PolygonMode::eFill, true);
    vk::Pipeline environment_render_pipeline = create_graphics_pipeline(dev,
        PATH_FROM_BINARY("shaders/environment_map.vert.spv"),
        PATH_FROM_BINARY("shaders/environment_map.frag.spv"),
        environment_render_pipeline_layout, render_pass, {},
        vk::PolygonMode::eFill, true);
    //////////////////////////////////
    ///* Initialization: Pipeline *///
    //////////////////////////////////

    ///////////////////////////////////////////
    ///* Initialization: Frame Sync Object *///
    ///////////////////////////////////////////
    uint32_t frame_counter = 0;
    std::array<vk::Fence, FRAME_IN_FLIGHT> graphics_fences{};
    std::array<vk::Semaphore, FRAME_IN_FLIGHT> graphics_semaphores{};
    std::array<vk::Semaphore, FRAME_IN_FLIGHT> present_semaphores{};
    {
        vk::FenceCreateInfo const fence_info{
            .flags = vk::FenceCreateFlagBits::eSignaled,
        };
        vk::SemaphoreCreateInfo const semaphore_info{};
        for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {
            VK_CHECK_CREATE(
                result, graphics_fences[i], dev.createFence(fence_info));
            VK_CHECK_CREATE(result, graphics_semaphores[i],
                dev.createSemaphore(semaphore_info));
            VK_CHECK_CREATE(result, present_semaphores[i],
                dev.createSemaphore(semaphore_info));
        }
    }
    ///////////////////////////////////////////
    ///* Initialization: Frame Sync Object *///
    ///////////////////////////////////////////

    vk::Sampler default_sampler = create_default_sampler(dev);

    /////////////////////////////////////////
    ///* Initialization: Environment Map *///
    /////////////////////////////////////////
    vma_image background_hdr_image;
    vma_image environment_map;
    {
        uint32_t const frame_sync_idx = frame_counter % FRAME_IN_FLIGHT;
        vk::Fence const fence = graphics_fences[frame_sync_idx];
        vk::CommandBuffer const graphics_command_buffer =
            graphics_command_buffers[frame_sync_idx];
        VK_CHECK(result, dev.waitForFences(1, &fence, true, 1e9));
        VK_CHECK(result, dev.resetFences(1, &fence));
        VK_CHECK(result, graphics_command_buffer.reset());
        vk::CommandBufferBeginInfo const begin_info{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        };
        VK_CHECK(result, graphics_command_buffer.begin(begin_info));
        background_hdr_image = create_texture2d_simple(dev, vma_alloc,
            graphics_command_buffer, background_hdr.width,
            background_hdr.height, vk::Format::eR32G32B32A32Sfloat, {},
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst);
        // the width and height of cubemap should equal according to the spec
        uint32_t const environment_map_size =
            std::max(background_hdr.width, background_hdr.height);
        environment_map = create_cubemap(dev, vma_alloc,
            graphics_command_buffer, environment_map_size, environment_map_size,
            vk::Format::eR32G32B32A32Sfloat, {});
        update_texture2d_simple(vma_alloc, background_hdr_image,
            graphics_command_buffer, background_hdr);
        update_descriptor_image_sampler_combined(dev,
            generate_cubemap_pipeline_set_0[0], 0, 0, default_sampler,
            background_hdr_image.primary_view);
        update_descriptor_storage_image(dev, generate_cubemap_pipeline_set_0[0],
            1, 0, environment_map.primary_view);
        graphics_command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute, generate_cubemap_pipeline);
        graphics_command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute, generate_cubemap_pipeline_layout,
            0, 1, &generate_cubemap_pipeline_set_0[0], 0, nullptr);
        graphics_command_buffer.dispatch(
            environment_map_size, environment_map_size, 6);
        VK_CHECK(result, graphics_command_buffer.end());
        vk::SubmitInfo const submit_info{
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &graphics_command_buffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };
        VK_CHECK(result, queues.graphics_queue.submit(1, &submit_info, fence));
        ++frame_counter;
    }
    /////////////////////////////////////////
    ///* Initialization: Environment Map *///
    /////////////////////////////////////////

    ///////////////////////////////////////
    ///* Initialization: Image, Buffer *///
    ///////////////////////////////////////
    create_dummy_buffer(vma_alloc);
    vma_buffer triangle_vertex_buffer{};
    vma_buffer triangle_face_buffer{};
    vma_buffer triangle_material_buffer{};
    vma_buffer point_light_buffer{};
    {
        uint32_t const frame_sync_idx = frame_counter % FRAME_IN_FLIGHT;
        vk::Fence const fence = graphics_fences[frame_sync_idx];
        vk::CommandBuffer const graphics_command_buffer =
            graphics_command_buffers[frame_sync_idx];
        VK_CHECK(result, graphics_command_buffer.reset());
        VK_CHECK(result, dev.waitForFences(1, &fence, true, 1e9));
        VK_CHECK(result, dev.resetFences(1, &fence));
        vk::CommandBufferBeginInfo const begin_info{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        };
        VK_CHECK(result, graphics_command_buffer.begin(begin_info));
        triangle_vertex_buffer = create_gpu_only_buffer(vma_alloc,
            size_in_byte(triangle_mesh.vertices), {},
            vk::BufferUsageFlagBits::eStorageBuffer);
        triangle_face_buffer = create_gpu_only_buffer(vma_alloc,
            size_in_byte(triangle_mesh.triangles), {},
            vk::BufferUsageFlagBits::eStorageBuffer);
        triangle_material_buffer = create_gpu_only_buffer(vma_alloc,
            size_in_byte(triangle_mesh.cook_torrance_materials), {},
            vk::BufferUsageFlagBits::eStorageBuffer);
        point_light_buffer =
            create_gpu_only_buffer(vma_alloc, size_in_byte(point_lights), {},
                vk::BufferUsageFlagBits::eStorageBuffer);
        update_buffer(vma_alloc, graphics_command_buffer,
            triangle_vertex_buffer, to_span(triangle_mesh.vertices), 0);
        update_buffer(vma_alloc, graphics_command_buffer, triangle_face_buffer,
            to_span(triangle_mesh.triangles), 0);
        update_buffer(vma_alloc, graphics_command_buffer,
            triangle_material_buffer,
            to_span(triangle_mesh.cook_torrance_materials), 0);
        update_buffer(vma_alloc, graphics_command_buffer, point_light_buffer,
            to_span(point_lights), 0);
        for (uint32_t frame_idx = 0; frame_idx < FRAME_IN_FLIGHT; ++frame_idx) {
            // primary render set 0
            update_descriptor_storage_buffer_whole(dev,
                primary_render_pipeline_set_0s[frame_idx], 0, 0,
                triangle_vertex_buffer);
            update_descriptor_storage_buffer_whole(dev,
                primary_render_pipeline_set_0s[frame_idx], 1, 0,
                triangle_face_buffer);
            // primary render set 1
            update_descriptor_storage_buffer_whole(dev,
                primary_render_pipeline_set_1s[frame_idx], 0, 0,
                triangle_material_buffer);
            update_descriptor_storage_buffer_whole(dev,
                primary_render_pipeline_set_1s[frame_idx], 1, 0,
                point_light_buffer);
            // enviroment render set 0
            update_descriptor_image_sampler_combined(dev,
                environment_render_pipeline_set_0s[frame_idx], 0, 0,
                default_sampler, environment_map.primary_view);
        }
        VK_CHECK(result, graphics_command_buffer.end());
        vk::SubmitInfo const graphics_submit_info{
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &graphics_command_buffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };
        VK_CHECK(result,
            queues.graphics_queue.submit(1, &graphics_submit_info, fence));
        ++frame_counter;
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
        vk::Fence const graphics_fence = graphics_fences[frame_sync_idx];
        vk::Semaphore const graphics_semaphore =
            graphics_semaphores[frame_sync_idx];
        vk::Semaphore const present_semaphore =
            present_semaphores[frame_sync_idx];
        vk::CommandBuffer const graphics_command_buffer =
            graphics_command_buffers[frame_sync_idx];
        vk::CommandBufferBeginInfo const begin_info{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        };
        std::array const primary_render_pipeline_sets{
            primary_render_pipeline_set_0s[frame_sync_idx],
            primary_render_pipeline_set_1s[frame_sync_idx],
        };
        std::array const environment_render_pipeline_sets{
            environment_render_pipeline_set_0s[frame_sync_idx],
        };
        primary_render_pipeline_vertex_push_constant const
            primary_render_pipeline_ps_vertex{
                .proj_view =
                    get_glsl_render_camera(camera, win_width, win_height),
            };
        primary_render_pipeline_fragment_push_constant const
            primary_render_pipeline_ps_fragment{
                .camera_position = glm::vec4{camera.position, 1.0f},
        };
        environment_render_pipeline_vertex_push_constant
            environment_render_pipeline_ps_vertex{
                .proj_view_for_environment_map =
                    get_glsl_render_camera_for_environment_map(
                        camera, win_width, win_height),
            };

        //////////////////
        ///* Graphics *///
        //////////////////
        VK_CHECK(result, dev.waitForFences(1, &graphics_fence, true, 1e9));
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
        VK_CHECK(result, dev.resetFences(1, &graphics_fence));
        VK_CHECK(result, graphics_command_buffer.reset());
        VK_CHECK(result, graphics_command_buffer.begin(begin_info));
        vk::Rect2D const render_area{
            .offset = {0, 0},
            .extent = swapchain_extent,
        };
        std::array const clear_values{
            vk::ClearValue{.color = {std::array{0.0f, 0.0f, 0.0f, 1.0f}}},
            vk::ClearValue{.depthStencil = {1.0f, 0}},
        };
        vk::RenderPassBeginInfo const render_pass_begin_info{
            .renderPass = render_pass,
            .framebuffer = framebuffers[swapchain_image_idx],
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
        // primary render
        graphics_command_buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics, primary_render_pipeline);
        graphics_command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, primary_render_pipeline_layout, 0,
            (uint32_t) primary_render_pipeline_sets.size(),
            primary_render_pipeline_sets.data(), 0, nullptr);
        graphics_command_buffer.pushConstants(primary_render_pipeline_layout,
            vk::ShaderStageFlagBits::eVertex, 0,
            (uint32_t) sizeof(primary_render_pipeline_ps_vertex),
            &primary_render_pipeline_ps_vertex);
        graphics_command_buffer.pushConstants(primary_render_pipeline_layout,
            vk::ShaderStageFlagBits::eFragment,
            (uint32_t) sizeof(primary_render_pipeline_ps_vertex),
            (uint32_t) sizeof(primary_render_pipeline_ps_fragment),
            &primary_render_pipeline_ps_fragment);
        graphics_command_buffer.draw(
            3 * (uint32_t) triangle_mesh.triangles.size(), 1, 0, 0);
        // enviroment render
        graphics_command_buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics, environment_render_pipeline);
        graphics_command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            environment_render_pipeline_layout, 0,
            (uint32_t) environment_render_pipeline_sets.size(),
            environment_render_pipeline_sets.data(), 0, nullptr);
        graphics_command_buffer.pushConstants(
            environment_render_pipeline_layout,
            vk::ShaderStageFlagBits::eVertex, 0,
            (uint32_t) sizeof(environment_render_pipeline_vertex_push_constant),
            &environment_render_pipeline_ps_vertex);
        graphics_command_buffer.draw(36, 1, 0, 0);
        graphics_command_buffer.endRenderPass();
        VK_CHECK(result, graphics_command_buffer.end());
        std::array const primary_render_wait_semaphores{
            present_semaphore,
        };
        std::array const primary_render_wait_stages{
            vk::PipelineStageFlags{
                vk::PipelineStageFlagBits::eColorAttachmentOutput},
        };
        vk::SubmitInfo const graphics_submit_info{
            .waitSemaphoreCount =
                (uint32_t) primary_render_wait_semaphores.size(),
            .pWaitSemaphores = primary_render_wait_semaphores.data(),
            .pWaitDstStageMask = primary_render_wait_stages.data(),
            .commandBufferCount = 1,
            .pCommandBuffers = &graphics_command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &graphics_semaphore,
        };
        VK_CHECK(result, queues.graphics_queue.submit(
                             1, &graphics_submit_info, graphics_fence));
        vk::PresentInfoKHR const present_info{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &graphics_semaphore,
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
        //////////////////
        ///* Graphics *///
        //////////////////

        glfwPollEvents();
        ++frame_counter;
    }

    /////////////////
    ///* Cleanup *///
    /////////////////
    std::free(background_hdr.data);
    glfwTerminate();
    VK_CHECK(result, dev.waitIdle());
    cleanup_staging_buffer(vma_alloc);
    cleanup_staging_image(vma_alloc);
    destroy_dummy_buffer(vma_alloc);
    dev.destroySampler(default_sampler);
    destroy_buffer(vma_alloc, triangle_vertex_buffer);
    destroy_buffer(vma_alloc, triangle_face_buffer);
    destroy_buffer(vma_alloc, triangle_material_buffer);
    destroy_buffer(vma_alloc, point_light_buffer);
    destroy_image(dev, vma_alloc, background_hdr_image);
    destroy_image(dev, vma_alloc, environment_map);
    dev.destroyDescriptorPool(default_descriptor_pool);
    for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {
        dev.destroyFence(graphics_fences[i]);
        dev.destroySemaphore(graphics_semaphores[i]);
        dev.destroySemaphore(present_semaphores[i]);
    }
    dev.destroyDescriptorSetLayout(environment_render_pipeline_set_0_layout);
    dev.destroyDescriptorSetLayout(primary_render_pipeline_set_0_layout);
    dev.destroyDescriptorSetLayout(primary_render_pipeline_set_1_layout);
    dev.destroyDescriptorSetLayout(generate_cubemap_pipeline_set_0_layout);
    dev.destroyPipelineLayout(environment_render_pipeline_layout);
    dev.destroyPipelineLayout(primary_render_pipeline_layout);
    dev.destroyPipelineLayout(generate_cubemap_pipeline_layout);
    dev.destroyPipeline(environment_render_pipeline);
    dev.destroyPipeline(primary_render_pipeline);
    dev.destroyPipeline(generate_cubemap_pipeline);
    dev.destroyCommandPool(graphics_command_pool);
    dev.destroyCommandPool(compute_command_pool);
    for (auto const framebuffer : framebuffers) {
        dev.destroyFramebuffer(framebuffer);
    }
    dev.destroyRenderPass(render_pass);
    destroy_image(dev, vma_alloc, color_buffer);
    destroy_image(dev, vma_alloc, depth_buffer);
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
