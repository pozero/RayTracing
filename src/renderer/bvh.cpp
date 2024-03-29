#include "check.h"
#include "bvh.h"
#include "utils/to_span.h"

#include <algorithm>

template <typename OBJ>
bvh_tree_node* build_bvh_recursive(std::vector<bvh_tree_node>& all_nodes,
    std::vector<OBJ> const& all_objects,
    std::span<bvh_primitive> bvh_primitives, uint32_t& sorted_object_offset,
    std::vector<OBJ>& sorted_objects);

static uint32_t flatten_bvh(std::vector<bvh_linear_node>& linear_nodes,
    bvh_tree_node* node, uint32_t& offset);

bvh create_bvh(scene const& scene) {
    CHECK(scene.vertices.size() % 3 == 0, "");
    std::vector<uint32_t> mesh_vertex_count{};
    std::vector<glsl_instance> instances{};
    std::vector<glsl_triangle> triangles{};
    std::vector<bvh_primitive> instance_bvh_primitives{};
    std::vector<bvh_primitive> triangle_bvh_primitives{};
    mesh_vertex_count.reserve(scene.mesh_vertex_start.size());
    instances.reserve(scene.primitives.size() + scene.lights.size());
    triangle_bvh_primitives.reserve(scene.vertices.size() / 3);
    instance_bvh_primitives.reserve(instances.size());
    for (uint32_t s = 1; s < scene.mesh_vertex_start.size(); ++s) {
        mesh_vertex_count.push_back(
            scene.mesh_vertex_start[s] - scene.mesh_vertex_start[s - 1]);
    }
    mesh_vertex_count.push_back(
        (uint32_t) scene.vertices.size() - scene.mesh_vertex_start.back());
    for (uint32_t p = 0; p < scene.primitives.size(); ++p) {
        primitive const& prim = scene.primitives[p];
        aabb const aabb = create_aabb(
            to_span(scene.vertices)
                .subspan(scene.mesh_vertex_start[(uint32_t) prim.mesh],
                    mesh_vertex_count[(uint32_t) prim.mesh]),
            scene.transformation[(uint32_t) prim.transform]);
        glsl_instance const inst{
            .mesh = prim.mesh,
            .transform = prim.transform,
            .material = prim.material,
            .medium = prim.medium,
            .light = -1,
        };
        instances.push_back(inst);
        instance_bvh_primitives.push_back(
            bvh_primitive{.aabb = aabb, .obj = p});
    }
    for (uint32_t l = 0; l < scene.lights.size(); ++l) {
        light const& light = scene.lights[l];
        if (light.type == light_type::distant) {
            continue;
        }
        aabb const aabb = create_aabb(
            to_span(scene.vertices)
                .subspan(scene.mesh_vertex_start[(uint32_t) light.mesh],
                    mesh_vertex_count[(uint32_t) light.mesh]),
            scene.transformation[(uint32_t) light.transform]);
        glsl_instance const inst{
            .mesh = (uint32_t) light.mesh,
            .transform = light.transform,
            .material = -1,
            .medium = -1,
            .light = (int32_t) l,
        };
        instances.push_back(inst);
        instance_bvh_primitives.push_back(bvh_primitive{
            .aabb = aabb, .obj = (uint32_t) instances.size() - 1});
    }
    for (uint32_t t = 0; t < scene.vertices.size() / 3; ++t) {
        aabb const aabb =
            create_aabb(to_span(scene.vertices).subspan(3 * t, 3));
        triangles.push_back(glsl_triangle{
            .a = scene.vertices[t],
            .b = scene.vertices[t + 1],
            .c = scene.vertices[t + 2],
        });
        triangle_bvh_primitives.push_back(
            bvh_primitive{.aabb = aabb, .obj = t});
    }

    // TLAS
    std::vector<bvh_tree_node> tlas_nodes{};
    std::vector<glsl_instance> sorted_instances{};
    tlas_nodes.reserve(2 * instance_bvh_primitives.size());
    sorted_instances.resize(instances.size());
    uint32_t sorted_instance_offset = 0;
    bvh_tree_node* root = build_bvh_recursive(tlas_nodes, instances,
        to_span(instance_bvh_primitives), sorted_instance_offset,
        sorted_instances);
    std::vector<bvh_linear_node> tlas_linear_nodes{};
    tlas_linear_nodes.resize(tlas_nodes.size());
    uint32_t tlas_linear_node_offset = 0;
    flatten_bvh(tlas_linear_nodes, root, tlas_linear_node_offset);
    // BLAS
    std::vector<bvh_tree_node> blas_nodes{};
    std::vector<glsl_triangle> sorted_triangles{};
    std::vector<bvh_linear_node> blas_linear_nodes{};
    std::vector<glsl_mesh> meshes{};
    blas_nodes.reserve(2 * triangle_bvh_primitives.size());
    sorted_triangles.resize(triangles.size());
    for (uint32_t m = 0; m < scene.mesh_vertex_start.size(); ++m) {
        uint32_t triangle_offset = scene.mesh_vertex_start[m] / 3;
        uint32_t const triangle_count = mesh_vertex_count[m] / 3;
        uint32_t const blas_node_count_before = (uint32_t) blas_nodes.size();
        bvh_tree_node* mesh_root = build_bvh_recursive(blas_nodes, triangles,
            to_span(triangle_bvh_primitives)
                .subspan(triangle_offset, triangle_count),
            triangle_offset, sorted_triangles);
        uint32_t blas_linear_node_offset = blas_node_count_before;
        blas_linear_nodes.resize(blas_nodes.size());
        flatten_bvh(blas_linear_nodes, mesh_root, blas_linear_node_offset);
        meshes.push_back(glsl_mesh{
            .triangle_offset = scene.mesh_vertex_start[m] / 3,
            .triangle_count = mesh_vertex_count[m] / 3,
            .bvh_start = blas_node_count_before,
        });
    }
    return bvh{tlas_linear_nodes, blas_linear_nodes, meshes, sorted_instances,
        sorted_triangles};
}

template <typename OBJ>
bvh_tree_node* build_bvh_recursive(std::vector<bvh_tree_node>& all_nodes,
    std::vector<OBJ> const& all_objects,
    std::span<bvh_primitive> bvh_primitives, uint32_t& sorted_object_offset,
    std::vector<OBJ>& sorted_objects) {
    CHECK(bvh_primitives.size() > 0, "");
    all_nodes.push_back(bvh_tree_node{});
    bvh_tree_node& node = all_nodes.back();
    aabb node_aabb{};
    for (auto const& prim : bvh_primitives) {
        node_aabb = combine_aabb(node_aabb, prim.aabb);
    }
    node.aabb = node_aabb;
    if (get_aabb_surface_area(node_aabb) == 0.0f ||
        bvh_primitives.size() == 1) {
        goto create_leaf;
    } else {
        aabb centroid_aabb{};
        for (auto const& prim : bvh_primitives) {
            centroid_aabb =
                combine_aabb(centroid_aabb, get_aabb_centroid(prim.aabb));
        }
        int32_t const largest_dim =
            (int32_t) get_aabb_largest_extent(centroid_aabb);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
        if (get_aabb_member(centroid_aabb, (uint32_t) largest_dim, 0) ==
            get_aabb_member(centroid_aabb, (uint32_t) largest_dim, 1)) {
#pragma clang diagnostic pop
            goto create_leaf;
        } else {
            uint32_t mid = (uint32_t) bvh_primitives.size() / 2;
            if (bvh_primitives.size() == 2) {
                if (get_aabb_centroid(bvh_primitives[0].aabb)[largest_dim] >
                    get_aabb_centroid(bvh_primitives[1].aabb)[largest_dim]) {
                    std::swap(bvh_primitives[0], bvh_primitives[1]);
                }
                mid = 1;
            } else {
                uint32_t constexpr NBUCKET = 12;
                std::array<uint32_t, NBUCKET> bucket_obj_counts{0};
                std::array<aabb, NBUCKET> bucket_aabbs{};
                for (auto const& prim : bvh_primitives) {
                    glm::vec3 const pmax = get_aabb_max(centroid_aabb);
                    glm::vec3 const pmin = get_aabb_min(centroid_aabb);
                    glm::vec3 const centroid = get_aabb_centroid(prim.aabb);
                    glm::vec3 const offset = (centroid - pmin) / (pmax - pmin);
                    uint32_t b = (uint32_t) (NBUCKET * offset[largest_dim]);
                    if (b == NBUCKET) {
                        b = NBUCKET - 1;
                    }
                    ++bucket_obj_counts[b];
                    bucket_aabbs[b] = combine_aabb(bucket_aabbs[b], prim.aabb);
                }
                uint32_t constexpr NSPLIT = NBUCKET - 1;
                std::array<float, NSPLIT> costs{0.0f};
                uint32_t count_below = 0;
                aabb aabb_below{};
                for (uint32_t i = 0; i < NSPLIT; ++i) {
                    count_below += bucket_obj_counts[i];
                    aabb_below = combine_aabb(aabb_below, bucket_aabbs[i]);
                    costs[i] +=
                        (float) count_below * get_aabb_surface_area(aabb_below);
                }
                uint32_t count_above = 0;
                aabb aabb_above{};
                for (uint32_t i = NSPLIT; i >= 1; --i) {
                    count_above += bucket_obj_counts[i];
                    aabb_above = combine_aabb(aabb_above, bucket_aabbs[i]);
                    costs[i - 1] +=
                        (float) count_above * get_aabb_surface_area(aabb_above);
                }
                uint32_t min_cost_split = 0;
                float min_cost = std::numeric_limits<float>::max();
                for (uint32_t i = 0; i < NSPLIT; ++i) {
                    if (min_cost < costs[i]) {
                        min_cost_split = i;
                        min_cost = costs[i];
                    }
                }
                float const leaf_cost = (float) bvh_primitives.size();
                min_cost = 0.5f + min_cost / get_aabb_surface_area(node_aabb);
                uint32_t constexpr MAX_PRIM = 3;
                if (bvh_primitives.size() > MAX_PRIM || min_cost < leaf_cost) {
                    auto iter = std::partition(bvh_primitives.begin(),
                        bvh_primitives.end(),
                        [=](bvh_primitive const& prim) -> bool {
                            glm::vec3 const pmax = get_aabb_max(centroid_aabb);
                            glm::vec3 const pmin = get_aabb_min(centroid_aabb);
                            glm::vec3 const centroid =
                                get_aabb_centroid(prim.aabb);
                            glm::vec3 const offset =
                                (centroid - pmin) / (pmax - pmin);
                            uint32_t b =
                                (uint32_t) (NBUCKET * offset[largest_dim]);
                            if (b == NBUCKET) {
                                b = NBUCKET - 1;
                            }
                            return b <= min_cost_split;
                        });
                    mid = (uint32_t) (iter - bvh_primitives.begin());
                    if (mid == 0 || mid == bvh_primitives.size()) {
                        for (uint32_t i = 0; i < NBUCKET; ++i) {
                            fmt::println("{}: {} ({}, {}) ({}, {}) ({}, {})", i,
                                bucket_obj_counts[i], bucket_aabbs[i].x_min,
                                bucket_aabbs[i].x_max, bucket_aabbs[i].y_min,
                                bucket_aabbs[i].y_max, bucket_aabbs[i].z_min,
                                bucket_aabbs[i].z_max);
                        }
                        CHECK(false, "");
                    }
                } else {
                    goto create_leaf;
                }
            }
            if (mid == 0 || mid == bvh_primitives.size()) {
                fmt::println("{} {}", mid, bvh_primitives.size());
                CHECK(false, "");
            }
            bvh_tree_node* left = build_bvh_recursive(all_nodes, all_objects,
                bvh_primitives.subspan(0, mid), sorted_object_offset,
                sorted_objects);
            bvh_tree_node* right = build_bvh_recursive(all_nodes, all_objects,
                bvh_primitives.subspan(mid, bvh_primitives.size() - mid),
                sorted_object_offset, sorted_objects);
            node.aabb = combine_aabb(left->aabb, right->aabb);
            node.left = left;
            node.right = right;
            node.split_axis = (bvh_split_axis) largest_dim;
            node.obj_count = 0;
            return &node;
        }
    }
create_leaf:
    for (uint32_t i = 0; i < bvh_primitives.size(); ++i) {
        uint32_t obj = bvh_primitives[i].obj;
        sorted_objects[sorted_object_offset + i] = all_objects[obj];
    }
    node.first_obj = sorted_object_offset;
    node.obj_count = (uint32_t) bvh_primitives.size();
    sorted_object_offset += bvh_primitives.size();
    return &node;
}

static uint32_t flatten_bvh(std::vector<bvh_linear_node>& linear_nodes,
    bvh_tree_node* node, uint32_t& offset) {
    bvh_linear_node& linear_node = linear_nodes[offset];
    linear_node.aabb = node->aabb;
    uint32_t const node_offset = offset;
    ++offset;
    if (node->obj_count > 0) {
        linear_node.first_obj = node->first_obj;
        linear_node.obj_count = node->obj_count;
    } else {
        linear_node.split_axis = node->split_axis;
        linear_node.obj_count = 0;
        flatten_bvh(linear_nodes, node->left, offset);
        linear_node.right = flatten_bvh(linear_nodes, node->right, offset);
    }
    return node_offset;
}
