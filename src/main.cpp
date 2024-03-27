#include "renderer/renderer.h"
#include "renderer/render_context.h"

#include "utils/file.h"
#include "utils/high_resolution_clock.h"
#include "asset/camera.h"
#include "asset/scene.h"

int main() {
    auto [render_options, camera, scene] =
        load_scene(PATH_FROM_ROOT("assets/hyperion_rect_light.json"));
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
