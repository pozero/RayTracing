#pragma once

#include "asset/mesh.h"
#include "asset/material.h"
#include "asset/texture.h"

struct scene {
    std::vector<mesh> meshes;
    std::vector<instance> instances;
    std::vector<material> materials;
    std::vector<texture_data> textures;

    std::vector<uint32_t> mesh_vertex_start;
};

void add_mesh(scene& scene, mesh const& mesh);

void add_material(scene& scene, material const& material);

void add_last_mesh_instance(scene& scene, glm::mat4 const& transformation);

void add_last_mesh_instance(
    scene& scene, glm::mat4 const& transformation, material const& material);

scene load_scene(std::string_view file_path);
