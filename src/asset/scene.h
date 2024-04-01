#pragma once

#include "asset/mesh.h"
#include "asset/material.h"
#include "asset/light.h"
#include "asset/texture.h"
#include "asset/camera.h"
#include "renderer/render_options.h"

struct scene {
    std::vector<vertex> vertices;
    std::vector<uint32_t> mesh_vertex_start;

    std::vector<texture_data> textures;
    std::vector<material> materials;
    std::vector<medium> mediums;

    std::vector<glm::mat4> transformation;
    std::vector<primitive> primitives;

    std::vector<light> lights;
};

std::tuple<render_options, camera, scene> load_scene(
    std::string_view file_path);
