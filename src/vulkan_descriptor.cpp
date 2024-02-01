#include "vulkan_descriptor.h"
#include "vulkan_check.h"

vk::DescriptorSetLayout create_descritpro_set_layout(vk::Device device,
    vk::ShaderStageFlagBits shader_stage,
    std::vector<vk_descriptor_set_binding> const& bindings) {
    vk::Result result;
    vk::DescriptorSetLayout descriptor_set_layout;
    std::vector<vk::DescriptorSetLayoutBinding> vk_bindings(bindings.size());
    for (uint32_t i = 0; i < bindings.size(); ++i) {
        vk_bindings[i].binding = i;
        vk_bindings[i].descriptorType = bindings[i].descriptor_type;
        vk_bindings[i].descriptorCount = bindings[i].array_count;
        vk_bindings[i].stageFlags = shader_stage;
    }
    vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_info{
        .bindingCount = (uint32_t) vk_bindings.size(),
        .pBindings = vk_bindings.data(),
    };
    VK_CHECK_CREATE(result, descriptor_set_layout,
        device.createDescriptorSetLayout(descriptor_set_layout_info));
    return descriptor_set_layout;
}

vk::PipelineLayout create_pipeline_layout(vk::Device device,
    std::vector<vk::DescriptorSetLayout> const& descriptor_set_layouts) {
    vk::Result result;
    vk::PipelineLayout pipeline_layout;
    vk::PipelineLayoutCreateInfo const pipeline_layout_info{
        .setLayoutCount = (uint32_t) descriptor_set_layouts.size(),
        .pSetLayouts = descriptor_set_layouts.data(),
        .pushConstantRangeCount = 0,
    };
    VK_CHECK_CREATE(result, pipeline_layout,
        device.createPipelineLayout(pipeline_layout_info));
    return pipeline_layout;
}

vk::PipelineLayout create_pipeline_layout(vk::Device device,
    uint32_t push_constant_size, vk::ShaderStageFlagBits push_constant_stage,
    std::vector<vk::DescriptorSetLayout> const& descriptor_set_layouts) {
    vk::Result result;
    vk::PipelineLayout pipeline_layout;
    vk::PushConstantRange const push_constant_range{
        .stageFlags = push_constant_stage,
        .offset = 0,
        .size = push_constant_size,
    };
    vk::PipelineLayoutCreateInfo const pipeline_layout_info{
        .setLayoutCount = (uint32_t) descriptor_set_layouts.size(),
        .pSetLayouts = descriptor_set_layouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant_range,
    };
    VK_CHECK_CREATE(result, pipeline_layout,
        device.createPipelineLayout(pipeline_layout_info));
    return pipeline_layout;
}
