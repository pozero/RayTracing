#include <vector>

#include "check.h"
#include "vulkan/vulkan_shader.h"
#include "utils/file.h"

vk::ShaderModule create_shader_module(
    vk::Device device, std::string_view file_path) {
    vk::Result result;
    vk::ShaderModule module;
    std::vector<char> const bytecode = read_binary(file_path);
    vk::ShaderModuleCreateInfo const shader_module_info{
        .codeSize = (uint32_t) bytecode.size(),
        .pCode = reinterpret_cast<const uint32_t *>(bytecode.data()),
    };
    VK_CHECK_CREATE(
        result, module, device.createShaderModule(shader_module_info));
    return module;
}

// GL_ARB_gpu_shader_int64
vk::SpecializationInfo create_specialization_info(
    std::vector<uint32_t> const &constants,
    std::vector<vk::SpecializationMapEntry> &entries) {
    entries.resize(constants.size());
    for (size_t i = 0; i < constants.size(); ++i) {
        entries[i].constantID = (uint32_t) i;
        entries[i].offset = (uint32_t) (i * sizeof(uint32_t));
        entries[i].size = sizeof(uint32_t);
    }
    vk::SpecializationInfo specialization_info{
        .mapEntryCount = (uint32_t) constants.size(),
        .pMapEntries = entries.data(),
        .dataSize = (uint32_t) constants.size() * sizeof(uint32_t),
        .pData = constants.data(),
    };
    return specialization_info;
}
