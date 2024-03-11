#include "check.h"
#include "vulkan/vulkan_descriptor.h"
#include "vulkan/vulkan_buffer.h"

vk::DescriptorSetLayout create_descriptor_set_layout(vk::Device device,
    vk::ShaderStageFlagBits shader_stage,
    std::vector<vk_descriptor_set_binding> const& bindings,
    vk::DescriptorSetLayoutCreateFlags flags) {
    vk::Result result;
    vk::DescriptorSetLayout descriptor_set_layout;
    std::vector<vk::DescriptorSetLayoutBinding> vk_bindings(bindings.size());
    std::vector<vk::DescriptorBindingFlags> vk_binding_flags(bindings.size());
    for (uint32_t i = 0; i < bindings.size(); ++i) {
        vk_bindings[i].binding = i;
        vk_bindings[i].descriptorType = bindings[i].descriptor_type;
        vk_bindings[i].descriptorCount = bindings[i].array_count;
        vk_bindings[i].stageFlags = shader_stage;
        vk_binding_flags[i] = bindings[i].flags;
    }
    vk::DescriptorSetLayoutBindingFlagsCreateInfo const binding_flags{
        .bindingCount = (uint32_t) vk_binding_flags.size(),
        .pBindingFlags = vk_binding_flags.data(),
    };
    vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_info{
        .pNext = &binding_flags,
        .flags = flags,
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
    std::vector<uint32_t> const& push_constant_sizes,
    std::vector<vk::ShaderStageFlagBits> const& push_constant_stages,
    std::vector<vk::DescriptorSetLayout> const& descriptor_set_layouts) {
    CHECK(push_constant_sizes.size() == push_constant_stages.size(), "");
    vk::Result result;
    vk::PipelineLayout pipeline_layout;
    std::vector<vk::PushConstantRange> push_constant_ranges{};
    uint32_t current_offset = 0;
    for (uint32_t i = 0; i < push_constant_sizes.size(); ++i) {
        push_constant_ranges.push_back(vk::PushConstantRange{
            .stageFlags = push_constant_stages[i],
            .offset = current_offset,
            .size = push_constant_sizes[i],
        });
        current_offset += push_constant_sizes[i];
    }
    vk::PipelineLayoutCreateInfo const pipeline_layout_info{
        .setLayoutCount = (uint32_t) descriptor_set_layouts.size(),
        .pSetLayouts = descriptor_set_layouts.data(),
        .pushConstantRangeCount = (uint32_t) push_constant_ranges.size(),
        .pPushConstantRanges = push_constant_ranges.data(),
    };
    VK_CHECK_CREATE(result, pipeline_layout,
        device.createPipelineLayout(pipeline_layout_info));
    return pipeline_layout;
}

vk::DescriptorPool create_descriptor_pool(
    vk::Device device, vk::DescriptorPoolCreateFlags flags) {
    vk::Result result;
    vk::DescriptorPool descriptor_pool;
    uint32_t constexpr MAX_SETS = 16;
    std::array const descriptor_pool_sizes{
        vk::DescriptorPoolSize{
                               .type = vk::DescriptorType::eCombinedImageSampler,
                               .descriptorCount = 50 * MAX_SETS,
                               },
        vk::DescriptorPoolSize{
                               .type = vk::DescriptorType::eStorageBuffer,
                               .descriptorCount = 50 * MAX_SETS,
                               },
        vk::DescriptorPoolSize{
                               .type = vk::DescriptorType::eUniformBuffer,
                               .descriptorCount = 50 * MAX_SETS,
                               },
    };
    vk::DescriptorPoolCreateInfo const pool_info{
        .flags = flags,
        .maxSets = MAX_SETS,
        .poolSizeCount = (uint32_t) descriptor_pool_sizes.size(),
        .pPoolSizes = descriptor_pool_sizes.data(),
    };
    VK_CHECK_CREATE(
        result, descriptor_pool, device.createDescriptorPool(pool_info));
    return descriptor_pool;
}

std::vector<vk::DescriptorSet> create_descriptor_set(vk::Device device,
    vk::DescriptorPool descriptor_pool,
    vk::DescriptorSetLayout descriptor_set_layout, uint32_t count,
    uint32_t variable_size) {
    vk::Result result;
    std::vector<vk::DescriptorSet> descriptor_sets{};
    std::vector<vk::DescriptorSetLayout> descriptor_set_layouts{
        count, descriptor_set_layout};
    std::vector<uint32_t> variable_sizes(count, variable_size);
    vk::DescriptorSetVariableDescriptorCountAllocateInfo const
        variable_count_info{
            .descriptorSetCount = count,
            .pDescriptorCounts = variable_sizes.data(),
        };
    vk::DescriptorSetAllocateInfo const allocate_info{
        .pNext = variable_size > 0 ? &variable_count_info : nullptr,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = count,
        .pSetLayouts = descriptor_set_layouts.data(),
    };
    VK_CHECK_CREATE(
        result, descriptor_sets, device.allocateDescriptorSets(allocate_info));
    return descriptor_sets;
}

void update_descriptor(vk::Device device, vk::DescriptorSet descriptor_set,
    uint32_t binding, uint32_t array_idx, vk::DescriptorType type,
    vk::DescriptorImageInfo const* image_info,
    vk::DescriptorBufferInfo const* buffer_info) {
    vk::WriteDescriptorSet const write_info{
        .dstSet = descriptor_set,
        .dstBinding = binding,
        .dstArrayElement = array_idx,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = image_info,
        .pBufferInfo = buffer_info,
    };
    device.updateDescriptorSets(1, &write_info, 0, nullptr);
}

void update_descriptor_image_sampler_combined(vk::Device device,
    vk::DescriptorSet descriptor_set, uint32_t binding, uint32_t array_idx,
    vk::Sampler sampler, vk::ImageView view) {
    vk::DescriptorImageInfo const image_info{
        .sampler = sampler,
        .imageView = view,
        .imageLayout = vk::ImageLayout::eGeneral,
    };
    update_descriptor(device, descriptor_set, binding, array_idx,
        vk::DescriptorType::eCombinedImageSampler, &image_info, nullptr);
}

void update_descriptor_storage_image(vk::Device device,
    vk::DescriptorSet descriptor_set, uint32_t binding, uint32_t array_idx,
    vk::ImageView view) {
    vk::DescriptorImageInfo const image_info{
        .imageView = view,
        .imageLayout = vk::ImageLayout::eGeneral,
    };
    update_descriptor(device, descriptor_set, binding, array_idx,
        vk::DescriptorType::eStorageImage, &image_info, nullptr);
}

void update_descriptor_storage_buffer_whole(vk::Device device,
    vk::DescriptorSet descriptor_set, uint32_t binding, uint32_t array_idx,
    vma_buffer const& buffer) {
    vk::DescriptorBufferInfo const buffer_info{
        .buffer = buffer.buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };
    update_descriptor(device, descriptor_set, binding, array_idx,
        vk::DescriptorType::eStorageBuffer, nullptr, &buffer_info);
}

void update_descriptor_uniform_buffer_whole(vk::Device device,
    vk::DescriptorSet descriptor_set, uint32_t binding, uint32_t array_idx,
    vma_buffer const& buffer) {
    vk::DescriptorBufferInfo const buffer_info{
        .buffer = buffer.buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };
    update_descriptor(device, descriptor_set, binding, array_idx,
        vk::DescriptorType::eUniformBuffer, nullptr, &buffer_info);
}
