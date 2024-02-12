#include "texture.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "stb_image.h"
#pragma clang diagnostic pop

#include "vulkan_check.h"

texture_data get_texture_data(std::string_view texture_path) {
    int width = 0, height = 0;
    int channels = 0;
    int const desired_channels = 4;
    uint8_t const* data = stbi_load(
        texture_path.data(), &width, &height, &channels, desired_channels);
    CHECK(data, "Can't load texture {}", texture_path);
    return texture_data{(uint32_t) width, (uint32_t) height,
        std::unique_ptr<uint8_t const>{data},
        desired_channels == 3 ? texture_channel::rgb : texture_channel::rgba};
}
