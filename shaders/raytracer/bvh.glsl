struct mesh_t {
    uint triangle_offset;
    uint triangle_count;
    uint bvh_start;
};

struct instance_t {
    uint mesh;
    int transform;
    int material;
    int medium;
    int light;
};

struct triangle_t {
    vertex_t a;
    vertex_t b;
    vertex_t c;
};

#define BVH_SPLIT_NONE -1
#define BVH_SPLIT_X 0
#define BVH_SPLIT_Y 1
#define BVH_SPLIT_Z 2

struct bvh_node_t {
    aabb_t aabb;
    uint right;
    uint first_obj;
    uint obj_count;
    int split_axis;
};
