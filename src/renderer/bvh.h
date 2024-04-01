#pragma once

#include "aabb.h"
#include "asset/scene.h"

#include <vector>

enum class bvh_split_axis : int32_t {
    none = -1,
    x,
    y,
    z
};

struct bvh_tree_node {
    aabb aabb{};
    bvh_tree_node* left = nullptr;
    bvh_tree_node* right = nullptr;
    // object can be instance or triangle for TLAS and BLAS, respectively
    uint32_t first_obj = 0;
    uint32_t obj_count = 0;
    bvh_split_axis split_axis = bvh_split_axis::none;
};

struct bvh_primitive {
    aabb aabb{};
    uint32_t obj = 0;
};

struct bvh_linear_node {
    aabb aabb{};
    uint32_t right = 0;
    uint32_t first_obj = 0;

    uint32_t obj_count = 0;
    bvh_split_axis split_axis = bvh_split_axis::none;
};

struct glsl_mesh {
    uint32_t triangle_offset;
    uint32_t triangle_count;
    uint32_t bvh_start;
};

struct glsl_instance {
    uint32_t mesh;
    int32_t transform;
    int32_t material;
    int32_t medium;
    int32_t light;
};

struct glsl_triangle {
    vertex a;
    vertex b;
    vertex c;
};

struct bvh {
    std::vector<bvh_linear_node> tlas{};
    std::vector<bvh_linear_node> blas{};
    std::vector<glsl_mesh> meshes{};
    std::vector<glsl_instance> instances{};
    std::vector<glsl_triangle> triangles{};
};

bvh create_bvh(scene const& scene);
