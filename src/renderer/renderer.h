#pragma once

struct renderer {
    void (*initialize)() = nullptr;

    void (*prepare_data)(struct scene const& scene) = nullptr;

    void (*update_data)(struct scene const& scene) = nullptr;

    void (*render)(struct camera const& camera) = nullptr;

    void (*present)() = nullptr;

    void (*destroy)() = nullptr;
};

void load_rasterizer(renderer& renderer);

void load_bvh_preview(renderer& renderer);
