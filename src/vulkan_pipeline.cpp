#include "vulkan_pipeline.h"
#include "vulkan_check.h"
#include "vulkan_swapchain.h"
#include "vulkan_shader.h"

vk::Pipeline create_graphics_pipeline(vk::Device device,
    std::string_view vert_path, std::string_view frag_path,
    vk::PipelineLayout layout, vk::RenderPass render_pass) {
    vk::Result result;
    vk::Pipeline pipeline;
    vk::ShaderModule vert_module = create_shader_module(device, vert_path);
    vk::ShaderModule frag_module = create_shader_module(device, frag_path);
    vk::PipelineShaderStageCreateInfo const vert_stage_info{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = vert_module,
        .pName = "main",
    };
    vk::PipelineShaderStageCreateInfo const frag_stage_info{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = frag_module,
        .pName = "main",
    };
    std::array const shader_stages{
        vert_stage_info,
        frag_stage_info,
    };
    std::array const dynamic_state{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo const dynamic_state_info{
        .dynamicStateCount = (uint32_t) dynamic_state.size(),
        .pDynamicStates = dynamic_state.data(),
    };
    vk::PipelineVertexInputStateCreateInfo const vertex_input_info{
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0,
    };
    vk::PipelineInputAssemblyStateCreateInfo const input_assembly_info{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False,
    };
    vk::Viewport const viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) swapchain_extent.width,
        .height = (float) swapchain_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vk::Rect2D const scissor{
        .offset = {0, 0},
        .extent = swapchain_extent,
    };
    vk::PipelineViewportStateCreateInfo const viewport_state_info{
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };
    vk::PipelineRasterizationStateCreateInfo const rasterization_info{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .depthBiasEnable = vk::False,
        .lineWidth = 1.0f,
    };
    vk::PipelineMultisampleStateCreateInfo const multisampling_info{
        .sampleShadingEnable = vk::False,
    };
    vk::PipelineColorBlendAttachmentState const blend_attachment_info{
        .blendEnable = vk::False,
        .colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };
    vk::PipelineColorBlendStateCreateInfo const blend_info{
        .logicOpEnable = vk::False,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment_info,
    };
    vk::GraphicsPipelineCreateInfo pipeline_info{
        .stageCount = (uint32_t) shader_stages.size(),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pViewportState = &viewport_state_info,
        .pRasterizationState = &rasterization_info,
        .pMultisampleState = &multisampling_info,
        .pColorBlendState = &blend_info,
        .pDynamicState = &dynamic_state_info,
        .layout = layout,
        .renderPass = render_pass,
        .subpass = 0,
    };
    VK_CHECK_CREATE(
        result, pipeline, device.createGraphicsPipeline({}, pipeline_info));
    device.destroyShaderModule(vert_module);
    device.destroyShaderModule(frag_module);
    return pipeline;
}
