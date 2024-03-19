#include "renderer/renderer.h"
#include "renderer/render_context.h"

#include "utils/high_resolution_clock.h"
#include "asset/camera.h"
#include "test_scene.h"

int main() {
    camera camera = create_camera(glm::vec3{0.0f, 0.0f, 5.0f},
        glm::vec3{0.0f, 0.0f, 0.0f}, 45.0f, win_width, win_height);
    scene const test_scene = brdf_parameter_test();
    renderer renderer{};
    load_rasterizer(renderer);
    create_render_context();
    renderer.initialize();
    renderer.prepare_data(test_scene);
    high_resolution_clock clock{};
    clock.tick();
    while (!window_should_close()) {
        clock.tick();
        update_camera(window, camera, clock.get_delta_seconds());
        renderer.update_data(test_scene);
        renderer.render(camera);
        submit_command_buffer(vk::PipelineBindPoint::eGraphics);
        renderer.present();
        poll_window_event();
    }
    wait_vulkan();
    renderer.destroy();
    destroy_render_context();
    return 0;
}
