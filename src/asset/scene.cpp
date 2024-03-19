#include "scene.h"

void add_mesh(scene& scene, mesh const& mesh) {
    uint32_t current_vertex_count = (uint32_t) scene.vertices.size();
    scene.vertices.insert(
        scene.vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
    scene.mesh_vertex_start.push_back(current_vertex_count);
}

void add_material(scene& scene, material const& material) {
    scene.materials.push_back(material);
}

void add_last_mesh_instance(scene& scene, glm::mat4 const& transformation) {
    uint32_t const mesh_idx = (uint32_t) scene.mesh_vertex_start.size() - 1;
    uint32_t const material_idx = (uint32_t) scene.materials.size() - 1;
    scene.instances.push_back(instance{
        .transformation = transformation,
        .mesh = mesh_idx,
        .material = material_idx,
    });
}

void add_last_mesh_instance(
    scene& scene, glm::mat4 const& transformation, material const& material) {
    uint32_t const mesh_idx = (uint32_t) scene.mesh_vertex_start.size() - 1;
    uint32_t const material_idx = (uint32_t) scene.materials.size();
    scene.instances.push_back(instance{
        .transformation = transformation,
        .mesh = mesh_idx,
        .material = material_idx,
    });
    scene.materials.push_back(material);
}
