#include "texture.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "stb_image.h"
#pragma clang diagnostic pop

#include "check.h"

texture_data get_texture_data(std::string_view texture_path) {
    int width = 0, height = 0;
    int channels = 0;
    int const desired_channels = 4;
    void* data = nullptr;
    bool const is_hdr = stbi_is_hdr(texture_path.data());
    if (is_hdr) {
        data = stbi_loadf(
            texture_path.data(), &width, &height, &channels, desired_channels);
    } else {
        data = stbi_load(
            texture_path.data(), &width, &height, &channels, desired_channels);
    }
    CHECK(data, "Can't load texture {}", texture_path);
    return texture_data{(uint32_t) width, (uint32_t) height, data,
        desired_channels == 3 ? texture_channel::rgb : texture_channel::rgba,
        is_hdr ? texture_format::sfloat : texture_format::unorm};
}
