#pragma once

#include "asset/scene.h"
#include "utils/file.h"

inline scene brdf_parameter_test() {
    scene scene{};
    mesh unit_sphere =
        load_mesh(PATH_FROM_ROOT("assets/freeform_objs/unit_sphere.obj"));
    return scene;
}
