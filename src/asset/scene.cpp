#include "scene.h"

void add_mesh(scene& scene, mesh const& mesh) {
    scene.meshes.push_back(mesh);
}

void add_material(scene& scene, material const& material) {
    scene.materials.push_back(material);
}

void add_last_mesh_instance(scene& scene, glm::mat4 const& transformation) {
    uint32_t const mesh_idx = (uint32_t) scene.meshes.size() - 1;
    uint32_t const material_idx = (uint32_t) scene.materials.size() - 1;
    scene.instances.push_back(instance{
        .transformation = transformation,
        .mesh = mesh_idx,
        .material = material_idx,
    });
}

void add_last_mesh_instance(
    scene& scene, glm::mat4 const& transformation, material const& material) {
    uint32_t const mesh_idx = (uint32_t) scene.meshes.size() - 1;
    uint32_t const material_idx = (uint32_t) scene.materials.size();
    scene.instances.push_back(instance{
        .transformation = transformation,
        .mesh = mesh_idx,
        .material = material_idx,
    });
    scene.materials.push_back(material);
}
