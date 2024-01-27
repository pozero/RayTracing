struct uniform_grid_t {
    vec3 starting_point;
    vec3 grid_size;
};

ivec3 world2grid(const in uniform_grid_t uniform_grid,
                  const in vec3 world_coord) {
    return ivec3((world_coord - uniform_grid.starting_point) / uniform_grid.grid_size);
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
