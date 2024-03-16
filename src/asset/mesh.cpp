#include "mesh.h"
#include "check.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "tiny_obj_loader.h"
#pragma clang diagnostic pop

mesh load_mesh(std::string_view file_path) {
    tinyobj::attrib_t attrib{};
    std::vector<tinyobj::shape_t> shapes{};
    std::vector<tinyobj::material_t> materials{};
    std::string warning{};
    std::string error{};
    bool const result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warning,
        &error, file_path.data(), NULL, true, true);
    CHECK(result,
        "Can't load obj file {}! TinyObj report warning: {}, error: {}",
        file_path, warning, error);
    mesh mesh{};
    auto const process_vertex = [&](size_t s, size_t f, size_t v) {
        tinyobj::index_t const idx = shapes[s].mesh.indices[f * 3 + v];
        tinyobj::real_t const vx = attrib.vertices[3 * idx.vertex_index + 0];
        tinyobj::real_t const vy = attrib.vertices[3 * idx.vertex_index + 1];
        tinyobj::real_t const vz = attrib.vertices[3 * idx.vertex_index + 2];
        tinyobj::real_t const nx = attrib.normals[3 * idx.normal_index + 0];
        tinyobj::real_t const ny = attrib.normals[3 * idx.normal_index + 1];
        tinyobj::real_t const nz = attrib.normals[3 * idx.normal_index + 2];
        tinyobj::real_t tx;
        tinyobj::real_t ty;
        if (attrib.texcoords.empty()) {
            if (v == 0) {
                tx = tinyobj::real_t{0};
                ty = tinyobj::real_t{0};
            } else if (v == 1) {
                tx = tinyobj::real_t{0};
                ty = tinyobj::real_t{1};
            } else {
                tx = tinyobj::real_t{1};
                ty = tinyobj::real_t{1};
            }
        } else {
            tx = attrib.texcoords[2 * idx.texcoord_index + 0];
            ty = attrib.texcoords[2 * idx.texcoord_index + 1];
        }
        mesh.vertices.push_back(vertex{
            .position_texu = glm::vec4{vx, vy, vz, tx},
            .normal_texv = glm::vec4{nx, ny, nz, ty}
        });
    };
    for (size_t s = 0; s < shapes.size(); ++s) {
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
            for (size_t v = 0; v < 3; ++v) {
                process_vertex(s, f, v);
            }
        }
    }
    return mesh;
}
