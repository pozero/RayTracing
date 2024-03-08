#include "raytracer/raytracer.h"

std::pair<uint32_t, uint32_t> get_frame_size1() {
    return std::make_pair(1366, 768);
}

std::tuple<camera, std::vector<glsl_sphere>, triangle_mesh,
    std::vector<texture_data>>
    get_scene1(uint32_t frame_width, uint32_t frame_height) {
    std::vector<glsl_sphere> spheres{};
    triangle_mesh mesh{};
    std::vector<texture_data> texture_datas{};
    glsl_material const material_ground =
        create_lambertian(glm::vec3{0.8f, 0.8f, 0.0f});
    // glsl_material const material_center =
    //     create_lambertian(glm::vec3{0.1f, 0.2f, 0.5f});
    glsl_material const material_center = create_lambertian(
        PATH_FROM_ROOT("assets/textures/earthmap.jpg"), texture_datas);
    glsl_material const material_left = create_dielectric(1.5f);
    glsl_material const material_right =
        create_metal(glm::vec3{0.8f, 0.6f, 0.2f}, 0.0f);
    spheres.push_back(create_sphere(
        glm::vec3{0.0f, -100.5f, -1.0f}, 100.0f, material_ground));
    spheres.push_back(
        create_sphere(glm::vec3{0.0f, 0.0f, -1.0f}, 0.5f, material_center));
    spheres.push_back(
        create_sphere(glm::vec3{-1.0f, 0.0f, -1.0f}, 0.5f, material_left));
    spheres.push_back(
        create_sphere(glm::vec3{-1.0f, 0.0f, -1.0f}, -0.4f, material_left));
    spheres.push_back(
        create_sphere(glm::vec3{1.0f, 0.0f, -1.0f}, 0.5f, material_right));
    camera const camera = create_camera(glm::vec3{-2.0f, 2.0f, 1.0f},
        glm::vec3{0.0f, 0.0f, -1.0f}, 20.0f, frame_width, frame_height);
    return std::make_tuple(camera, spheres, mesh, std::move(texture_datas));
}

glsl_sky_color get_sky_color1() {
    return glsl_sky_color{
        .sky_color_top = {0.5f, 0.7f, 1.0f},
        .sky_color_bottom = {1.0f, 1.0f, 1.0f},
    };
}

std::pair<uint32_t, uint32_t> get_frame_size2() {
    return std::make_pair(800, 800);
}

std::tuple<camera, std::vector<glsl_sphere>, triangle_mesh,
    std::vector<texture_data>>
    get_scene2(uint32_t frame_width, uint32_t frame_height) {
    std::vector<glsl_sphere> spheres{};
    triangle_mesh mesh{};
    std::vector<texture_data> texture_datas{};
    glsl_material const left_red =
        create_lambertian(glm::vec3{1.0f, 0.2f, 0.2f});
    glsl_material const back_green =
        create_lambertian(glm::vec3{0.2f, 1.0f, 0.2f});
    glsl_material const right_blue =
        create_lambertian(glm::vec3{0.2f, 0.2f, 1.0f});
    glsl_material const upper_orange =
        create_lambertian(glm::vec3{1.0f, 0.5f, 0.0f});
    glsl_material const lower_teal =
        create_lambertian(glm::vec3{0.2f, 0.8f, 0.8f});
    add_quad(mesh, glm::vec3{-3.0f, -2.0f, 5.0f}, glm::vec3{0.0f, 0.0f, -4.0f},
        glm::vec3{0.0f, 4.0f, 0.0f}, left_red);
    add_quad(mesh, glm::vec3{-2.0f, -2.0f, 0.0f}, glm::vec3{4.0f, 0.0f, 0.0f},
        glm::vec3{0.0f, 4.0f, 0.0f}, back_green);
    add_quad(mesh, glm::vec3{3.0f, -2.0f, 1.0f}, glm::vec3{0.0f, 0.0f, 4.0f},
        glm::vec3{0.0f, 4.0f, 0.0f}, right_blue);
    add_quad(mesh, glm::vec3{-2.0f, 3.0f, 1.0f}, glm::vec3{4.0f, 0.0f, 0.0f},
        glm::vec3{0.0f, 0.0f, 4.0f}, upper_orange);
    add_quad(mesh, glm::vec3{-2.0f, -3.0f, 5.0f}, glm::vec3{4.0f, 0.0f, 0.0f},
        glm::vec3{0.0f, 0.0f, -4.0f}, lower_teal);
    camera const camera = create_camera(glm::vec3{0.0f, 0.0f, 9.0f},
        glm::vec3{0.0f, 0.0f, 0.0f}, 80.0f, frame_width, frame_height);
    return std::make_tuple(camera, spheres, mesh, std::move(texture_datas));
}

glsl_sky_color get_sky_color2() {
    return glsl_sky_color{
        .sky_color_top = {0.5f, 0.7f, 1.0f},
        .sky_color_bottom = {1.0f, 1.0f, 1.0f},
    };
}

std::pair<uint32_t, uint32_t> get_frame_size3() {
    return std::make_pair(800, 800);
}

std::tuple<camera, std::vector<glsl_sphere>, triangle_mesh,
    std::vector<texture_data>>
    get_scene3(uint32_t frame_width, uint32_t frame_height) {
    std::vector<glsl_sphere> spheres{};
    triangle_mesh mesh{};
    std::vector<texture_data> texture_datas{};
    glsl_material const red = create_lambertian(glm::vec3{0.65f, 0.05f, 0.05f});
    glsl_material const white =
        create_lambertian(glm::vec3{0.73f, 0.73f, 0.73f});
    glsl_material const green =
        create_lambertian(glm::vec3{0.12f, 0.45f, 0.15f});
    glsl_material const light =
        create_diffuse_light(glm::vec3{15.0f, 15.0f, 15.0f});
    add_quad(mesh, glm::vec3{555.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 555.0f, 0.0f},
        glm::vec3{0.0f, 0.0f, 555.0f}, green);
    add_quad(mesh, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 555.0f, 0.0f},
        glm::vec3{0.0f, 0.0f, 555.0f}, red);
    add_quad(mesh, glm::vec3{343.0f, 554.0f, 332.0f},
        glm::vec3{-130.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, -105.0f}, light);
    add_quad(mesh, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{555.0f, 0.0f, 0.0f},
        glm::vec3{0.0f, 0.0f, 555.0f}, white);
    add_quad(mesh, glm::vec3{555.0f, 555.0f, 555.0f},
        glm::vec3{-555.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, -555.0f}, white);
    add_quad(mesh, glm::vec3{0.0f, 0.0f, 555.0f}, glm::vec3{555.0f, 0.0f, 0.0f},
        glm::vec3{0.0f, 555.0f, 0.0f}, white);
    camera const camera = create_camera(glm::vec3{278.0f, 278.0f, -800.0f},
        glm::vec3{278.0f, 278.0f, 0.0f}, 40.0f, frame_width, frame_height);
    return std::make_tuple(camera, spheres, mesh, std::move(texture_datas));
}

glsl_sky_color get_sky_color3() {
    return glsl_sky_color{
        .sky_color_top = glm::vec3{0.0f},
        .sky_color_bottom = glm::vec3{0.0f},
    };
}
