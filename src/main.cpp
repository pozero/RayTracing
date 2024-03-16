#include "renderer/renderer.h"
#include "renderer/render_context.h"

int main() {
    renderer renderer{};
    load_rasterizer(renderer);
    bool running = true;
    create_render_context();
    while (running) {}
    destroy_render_context();
    return 0;
}
