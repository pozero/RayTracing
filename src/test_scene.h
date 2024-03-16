#pragma once

#include "asset/scene.h"
#include "utils/file.h"

inline scene brdf_parameter_test() {
    scene scene{};
    mesh unit_sphere =
        load_mesh(PATH_FROM_ROOT("assets/freeform_objs/unit_sphere.obj"));
    add_mesh(scene, unit_sphere);
    add_last_mesh_instance(scene, glm::mat4{1.0f}, material{});
    return scene;
}
