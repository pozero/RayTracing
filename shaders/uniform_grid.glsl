layout(constant_id = 1) const uint GRID_ALONG_X_AXIS = 1;
layout(constant_id = 2) const uint GRID_ALONG_Y_AXIS = 1;
layout(constant_id = 3) const uint GRID_ALONG_Z_AXIS = 1;

struct uniform_grid_t {
    vec3 starting_point;
    vec3 grid_size;
    vec3 grid_size_inv;
};

uniform_grid_t create_uniform_grid(const in aabb_t world_aabb) {
    const vec3 starting_point = aabb_lowest_point(world_aabb);
    const vec3 inv_count = vec3(1.0, 1.0, 1.0) / vec3(GRID_ALONG_X_AXIS,
                                                      GRID_ALONG_Y_AXIS,
                                                      GRID_ALONG_Z_AXIS);
    const vec3 world_aabb_size = vec3(world_aabb.x_interval.y - world_aabb.x_interval.x,
                                      world_aabb.y_interval.y - world_aabb.y_interval.x,
                                      world_aabb.z_interval.y - world_aabb.z_interval.x);
    const vec3 grid_size = inv_count * world_aabb_size;
    const vec3 grid_size_inv = vec3(1.0, 1.0, 1.0) / grid_size;
    return uniform_grid_t(
        starting_point,
        grid_size,
        grid_size_inv);
}

ivec3 world2grid(const in uniform_grid_t uniform_grid,
                  const in vec3 world_coord) {
    return ivec3((world_coord - uniform_grid.starting_point) * uniform_grid.grid_size_inv);
}

vec3 grid2world(const in uniform_grid_t uniform_grid,
                 const in ivec3 grid_coord) {
    return grid_coord * uniform_grid.grid_size + uniform_grid.starting_point;
}

aabb_t get_grid_aabb(const in uniform_grid_t uniform_grid,
                      const in ivec3 grid_coord) {
    const vec3 lowest_point = grid2world(uniform_grid, grid_coord);
    const vec3 highest_point = grid2world(uniform_grid, grid_coord + ivec3(1, 1, 1));
    return aabb_t(
        vec2(lowest_point.x, highest_point.x),
        vec2(lowest_point.y, highest_point.y),
        vec2(lowest_point.z, highest_point.z)
    );
}

uint flatten_grid_coord(const in ivec3 grid_coord) {
    return grid_coord.x +
           grid_coord.y * GRID_ALONG_X_AXIS +
           grid_coord.z * GRID_ALONG_X_AXIS * GRID_ALONG_Y_AXIS;
}
