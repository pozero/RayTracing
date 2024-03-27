#pragma once

#include "aabb.h"
#include "asset/scene.h"

#include <vector>
#include <tuple>

enum class bvh_split_axis : uint32_t {
    x,
    y,
    z
};

struct bvh_tree_node {
    aabb box{};
    bvh_tree_node* left = nullptr;
    bvh_tree_node* right = nullptr;
    // object can be instance or triangle for TLAS and BLAS, respectively
    uint32_t first_obj = 0;
    uint32_t obj_count = 0;
    bvh_split_axis split_axis;
};

struct bvh_linear_node {
    aabb box{};
    int32_t right = -1;
    uint32_t first_obj = 0;
    uint32_t obj_count = 0;
    bvh_split_axis split_axis;
};

struct glsl_instance {
    int32_t mesh;
    int32_t transform;
    int32_t material;
    int32_t medium;
    int32_t light;
};

// this function would sort the vertices inside scene to match the BLAS
std::tuple<std::vector<bvh_linear_node>, std::vector<glsl_instance>> create_bvh(
    scene& scene);
