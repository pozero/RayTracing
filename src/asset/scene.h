#pragma once

#include "mesh.h"
#include "material.h"

struct scene {
    std::vector<mesh> meshes;
    std::vector<instance> instances;
    std::vector<material> materials;
};

void add_mesh(scene& scene, mesh const& mesh);

void add_material(scene& scene, material const& material);

void add_last_mesh_instance(scene& scene, glm::mat4 const& transformation);

void add_last_mesh_instance(
    scene& scene, glm::mat4 const& transformation, material const& material);

scene load_scene(std::string_view file_path);
