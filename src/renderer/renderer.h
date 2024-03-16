#pragma once

struct renderer {
    void (*initialize)() = nullptr;

    void (*prepare_data)(struct scene const& scene) = nullptr;

    void (*update_data)(
        struct camera const& camera, struct scene const& scene) = nullptr;

    void (*render)() = nullptr;

    void (*destroy)() = nullptr;
};

void load_rasterizer(renderer& renderer);
