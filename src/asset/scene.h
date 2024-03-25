#pragma once

#include "asset/mesh.h"
#include "asset/material.h"
#include "asset/light.h"
#include "asset/texture.h"

struct scene {
    // meshes
    std::vector<vertex> vertices;
    std::vector<instance> instances;
    std::vector<material> materials;
    std::vector<medium> mediums;
    std::vector<texture_data> textures;

    std::vector<uint32_t> mesh_vertex_start;

    std::vector<light> lights;
    sky_light sky_light;
};

scene load_scene(std::string_view file_path);
