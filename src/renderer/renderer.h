#pragma once

#include "renderer/render_options.h"

struct renderer {
    void (*initialize)(render_options const& options) = nullptr;

    void (*prepare_data)(struct scene const& scene) = nullptr;

    void (*update_data)(struct scene const& scene) = nullptr;

    void (*render)(struct camera const& camera) = nullptr;

    void (*present)() = nullptr;

    void (*destroy)() = nullptr;
};

void load_rasterizer(renderer& renderer);

void load_bvh_preview(renderer& renderer);

void load_megakernel_raytracer(renderer& renderer);
