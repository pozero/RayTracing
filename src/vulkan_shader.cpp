#include <vector>

#include "vulkan_shader.h"
#include "vulkan_check.h"
#include "file.h"

vk::ShaderModule create_shader_module(
    vk::Device device, std::string_view file_path) {
    vk::Result result;
    vk::ShaderModule module;
    std::vector<char> const bytecode = read_binary(file_path);
    vk::ShaderModuleCreateInfo const shader_module_info{
        .codeSize = (uint32_t) bytecode.size(),
        .pCode = reinterpret_cast<const uint32_t*>(bytecode.data()),
    };
    VK_CHECK_CREATE(
        result, module, device.createShaderModule(shader_module_info));
    return module;
}
