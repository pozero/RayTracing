#include "renderer/renderer.h"
#include "renderer/render_context.h"
#include "renderer/bvh.h"

#include "utils/file.h"
#include "utils/high_resolution_clock.h"
#include "asset/camera.h"
#include "asset/scene.h"

#pragma clang diagnostic ignored "-Weverything"
#include "fmt/core.h"

int main() {
    auto [render_options, camera, scene] =
        load_scene(PATH_FROM_ROOT("assets/hyperion_rect_light.json"));
    bvh const bvh = create_bvh(scene);
    for (auto const& node : bvh.tlas) {
        fmt::println(
            "aabb: ({}, {}) ({}, {}) ({}, {}) right: {}, first_obj: {}, "
            "obj_count: {}, split_axis: {}",
            node.aabb.x_min, node.aabb.x_max, node.aabb.y_min, node.aabb.y_max,
            node.aabb.z_min, node.aabb.z_max, node.right, node.first_obj,
            node.obj_count, (int) node.split_axis);
    }
    fmt::println("===============================");
    for (auto const& node : bvh.blas) {
        fmt::println(
            "aabb: ({}, {}) ({}, {}) ({}, {}) right: {}, first_obj: {}, "
            "obj_count: {}, split_axis: {}",
            node.aabb.x_min, node.aabb.x_max, node.aabb.y_min, node.aabb.y_max,
            node.aabb.z_min, node.aabb.z_max, node.right, node.first_obj,
            node.obj_count, (int) node.split_axis);
    }
    // win_width = render_options.resolution_x;
    // win_height = render_options.resolution_y;
    // renderer renderer{};
    // load_rasterizer(renderer);
    // create_render_context();
    // renderer.initialize();
    // renderer.prepare_data(scene);
    // high_resolution_clock clock{};
    // clock.tick();
    // while (!window_should_close()) {
    //     clock.tick();
    //     update_camera(window, camera, clock.get_delta_seconds());
    //     renderer.update_data(scene);
    //     renderer.render(camera);
    //     submit_command_buffer(vk::PipelineBindPoint::eGraphics);
    //     renderer.present();
    //     poll_window_event();
    // }
    // wait_vulkan();
    // renderer.destroy();
    // destroy_render_context();
    for (auto const& t : scene.textures) {
        free(t.data);
    }
    return 0;
}
