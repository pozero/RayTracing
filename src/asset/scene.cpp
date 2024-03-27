#include "scene.h"
#include "check.h"
#include "asset/texture.h"

#include <tuple>
#include <fstream>
#include <unordered_map>
#include <filesystem>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "nlohmann/json.hpp"
#include "glm/gtx/quaternion.hpp"
#pragma clang diagnostic pop

#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"

std::tuple<render_options, camera, scene> load_scene(
    std::string_view file_path) {
    try {
        std::ifstream ifs{file_path.data()};
        nlohmann::json root_json = nlohmann::json::parse(ifs);
        // render options
        render_options const options{
            .resolution_x = root_json.at("/renderer/resolution/0"_json_pointer),
            .resolution_y = root_json.at("/renderer/resolution/1"_json_pointer),
            .max_depth = root_json.at("/renderer/max_depth"_json_pointer),
        };
        // camera
        glm::vec3 const lookfrom{
            root_json.at("/camera/lookfrom/0"_json_pointer),
            root_json.at("/camera/lookfrom/1"_json_pointer),
            root_json.at("/camera/lookfrom/2"_json_pointer),
        };
        glm::vec3 const lookat{
            root_json.at("/camera/lookat/0"_json_pointer),
            root_json.at("/camera/lookat/1"_json_pointer),
            root_json.at("/camera/lookat/2"_json_pointer),
        };
        float const fov = root_json.at("/camera/fov"_json_pointer);
        camera const camera = create_camera(
            lookfrom, lookat, fov, options.resolution_x, options.resolution_y);
        // scene
        scene scene{};
        std::filesystem::path const cur_dir =
            std::filesystem::path{file_path}.parent_path();
        std::unordered_map<std::filesystem::path, int32_t> mesh_indices{};
        std::unordered_map<std::filesystem::path, int32_t> texture_indices{};
        std::unordered_map<std::string, int32_t> material_indices{};
        std::unordered_map<std::string, int32_t> medium_indices{};
        auto const get_mesh = [&cur_dir, &mesh_indices, &scene](
                                  std::string const& path) -> int32_t {
            int32_t id = -1;
            std::filesystem::path full_path =
                cur_dir / std::filesystem::path{path};
            if (auto const iter = mesh_indices.find(path);
                iter != mesh_indices.end()) {
                id = iter->second;
            } else {
                mesh const mesh = load_mesh(full_path.c_str());
                scene.mesh_vertex_start.push_back(
                    (uint32_t) scene.vertices.size());
                scene.vertices.insert(scene.vertices.end(),
                    mesh.vertices.begin(), mesh.vertices.end());
                id = (int32_t) scene.mesh_vertex_start.size() - 1;
                mesh_indices[full_path] = id;
            }
            return id;
        };
        auto const get_texture = [&cur_dir, &texture_indices, &scene](
                                     std::string const& path) -> int32_t {
            int32_t id = -1;
            std::filesystem::path full_path =
                cur_dir / std::filesystem::path{path};
            auto iter = texture_indices.find(full_path);
            if (iter != texture_indices.end()) {
                id = iter->second;
            } else {
                texture_data const data = get_texture_data(full_path.c_str());
                scene.textures.push_back(data);
                id = (int32_t) scene.textures.size() - 1;
                texture_indices[full_path] = id;
            }
            return id;
        };
        auto const& mat_json = root_json.at("/material"_json_pointer);
        for (auto const& [key, val] : mat_json.items()) {
            glm::vec3 const albedo{
                val.value<float>("/albedo/0"_json_pointer, 1.0f),
                val.value<float>("/albedo/1"_json_pointer, 1.0f),
                val.value<float>("/albedo/2"_json_pointer, 1.0f),
            };
            glm::vec3 const emission{
                val.value<float>("/emission/0"_json_pointer, 0.0f),
                val.value<float>("/emission/1"_json_pointer, 0.0f),
                val.value<float>("/emission/2"_json_pointer, 0.0f),
            };
            float const metallic =
                val.value<float>("/metallic"_json_pointer, 0.0f);
            float const spec_trans =
                val.value<float>("/spec_trans"_json_pointer, 0.0f);
            float const ior = val.value<float>("/ior"_json_pointer, 1.5f);
            float const subsurface =
                val.value<float>("/subsurface"_json_pointer, 0.0f);
            float const roughness =
                val.value<float>("/roughness"_json_pointer, 0.5f);
            float const specular_tint =
                val.value<float>("/specular_tint"_json_pointer, 0.0f);
            float const anisotropic =
                val.value<float>("/anisotropic"_json_pointer, 0.0f);
            float const sheen = val.value<float>("/sheen"_json_pointer, 0.0f);
            float const sheen_tint =
                val.value<float>("/sheen_tint"_json_pointer, 0.5f);
            float const clearcoat =
                val.value<float>("/clearcoat"_json_pointer, 0.0f);
            float const clearcoat_gloss =
                val.value<float>("/clearcoat_gloss"_json_pointer, 1.0f);
            std::array textures{
                val.value<std::string>("/albedo_tex"_json_pointer, ""),
                val.value<std::string>("/emission_tex"_json_pointer, ""),
                val.value<std::string>("/normal_tex"_json_pointer, ""),
                val.value<std::string>(
                    "/metallic_roughness_tex"_json_pointer, ""),
            };
            std::array tex_ids{-1, -1, -1, -1};
            for (uint32_t i = 0; i < textures.size(); ++i) {
                if (!textures[i].empty()) {
                    tex_ids[i] = get_texture(textures[i]);
                }
            }
            material const mat{
                .albedo = albedo,
                .emission = emission,
                .metallic = metallic,
                .spec_trans = spec_trans,
                .ior = ior,
                .subsurface = subsurface,
                .roughness = roughness,
                .specular_tint = specular_tint,
                .anisotropic = anisotropic,
                .sheen = sheen,
                .sheen_tint = sheen_tint,
                .clearcoat = clearcoat,
                .clearcoat_gloss = clearcoat_gloss,
                .albedo_tex = tex_ids[0],
                .emission_tex = tex_ids[1],
                .normal_tex = tex_ids[2],
                .metallic_roughness_tex = tex_ids[3],
            };
            scene.materials.push_back(mat);
            material_indices[key] = (int32_t) scene.materials.size() - 1;
        }
        std::array const medium_types{
            medium_type::absorption,
            medium_type::emission,
            medium_type::scattering,
        };
        std::array const medium_type_keys{
            "/medium/absorption"_json_pointer,
            "/medium/emission"_json_pointer,
            "/medium/anisotropic"_json_pointer,
        };
        for (uint32_t i = 0; i < medium_types.size(); ++i) {
            if (root_json.contains(medium_type_keys[i])) {
                auto const& med_json = root_json.at(medium_type_keys[i]);
                for (auto const& [key, val] : med_json.items()) {
                    glm::vec3 const color{
                        val.at("/color/0"_json_pointer),
                        val.at("/color/1"_json_pointer),
                        val.at("/color/2"_json_pointer),
                    };
                    float const density = val.at("/density"_json_pointer);
                    float const anisotropic =
                        val.at("/anisotropic"_json_pointer);
                    medium const med{
                        .color = color,
                        .density = density,
                        .anisotropic = anisotropic,
                        .type = medium_types[i],
                    };
                    scene.mediums.push_back(med);
                    medium_indices[key] = (int32_t) scene.mediums.size() - 1;
                }
            }
        }
        auto const& prim_json = root_json.at("/primitive"_json_pointer);
        for (auto const& [key, val] : prim_json.items()) {
            std::string const mesh_file = val.at("/mesh"_json_pointer);
            int32_t mesh_idx = get_mesh(mesh_file);
            int32_t material_idx = -1;
            if (val.contains("/material"_json_pointer)) {
                material_idx =
                    material_indices.at(val.at("/material"_json_pointer));
            }
            int32_t medium_idx = -1;
            if (val.contains("/medium"_json_pointer)) {
                medium_idx = medium_indices.at(val.at("/medium"_json_pointer));
            }
            glm::mat4 transform{1.0f};
            if (val.contains("/position"_json_pointer)) {
                glm::vec3 const position{
                    val.at("/position/0"_json_pointer),
                    val.at("/position/1"_json_pointer),
                    val.at("/position/2"_json_pointer),
                };
                transform = glm::translate(transform, position);
            }
            if (val.contains("/rotation"_json_pointer)) {
                glm::quat const quaterion{
                    val.at("/rotation/0"_json_pointer),
                    val.at("/rotation/1"_json_pointer),
                    val.at("/rotation/2"_json_pointer),
                    val.at("/rotation/3"_json_pointer),
                };
                transform *= glm::toMat4(quaterion);
            }
            if (val.contains("/scale"_json_pointer)) {
                glm::vec3 const scale{
                    val.at("/scale/0"_json_pointer),
                    val.at("/scale/1"_json_pointer),
                    val.at("/scale/2"_json_pointer),
                };
                transform = glm::scale(transform, scale);
            }
            scene.transformation.push_back(transform);
            primitive const inst{
                .mesh = mesh_idx,
                .transform = (int32_t) scene.transformation.size() - 1,
                .material = material_idx,
                .medium = medium_idx,
            };
            scene.primitives.push_back(inst);
        }
        auto const& area_light_json = root_json.at("/light/area"_json_pointer);
        for (auto const& [key, val] : area_light_json.items()) {
            glm::vec3 const intensity{
                val.at("/intensity/0"_json_pointer),
                val.at("/intensity/1"_json_pointer),
                val.at("/intensity/2"_json_pointer),
            };
            int32_t emission_id = -1;
            if (val.contains("/emission_tex"_json_pointer)) {
                std::string const emission_tex_file =
                    val.at("/emission_tex"_json_pointer);
                emission_id = get_texture(emission_tex_file);
            }
            int32_t const mesh_id =
                mesh_indices.at(val.at("/mesh"_json_pointer));
            glm::mat4 transform{1.0f};
            if (val.contains("/position"_json_pointer)) {
                glm::vec3 const position{
                    val.at("/position/0"_json_pointer),
                    val.at("/position/1"_json_pointer),
                    val.at("/position/2"_json_pointer),
                };
                transform = glm::translate(transform, position);
            }
            if (val.contains("/rotation"_json_pointer)) {
                glm::quat const quaterion{
                    val.at("/rotation/0"_json_pointer),
                    val.at("/rotation/1"_json_pointer),
                    val.at("/rotation/2"_json_pointer),
                    val.at("/rotation/3"_json_pointer),
                };
                transform *= glm::toMat4(quaterion);
            }
            if (val.contains("/scale"_json_pointer)) {
                glm::vec3 const scale{
                    val.at("/scale/0"_json_pointer),
                    val.at("/scale/1"_json_pointer),
                    val.at("/scale/2"_json_pointer),
                };
                transform = glm::scale(transform, scale);
            }
            scene.transformation.push_back(transform);
            light const light{
                .intensity = intensity,
                .direction = {},
                .mesh = mesh_id,
                .transform = (int32_t) scene.transformation.size() - 1,
                .emission_tex = emission_id,
                .type = light_type::area,
            };
            scene.lights.push_back(light);
        }
        auto const& distant_light_json =
            root_json.at("/light/distant"_json_pointer);
        for (auto const& [key, val] : distant_light_json.items()) {
            glm::vec3 const intensity{
                val.at("/intensity/0"_json_pointer),
                val.at("/intensity/1"_json_pointer),
                val.at("/intensity/2"_json_pointer),
            };
            glm::vec3 const direction{
                val.at("/direction/0"_json_pointer),
                val.at("/direction/1"_json_pointer),
                val.at("/direction/2"_json_pointer),
            };
            light const light{
                .intensity = intensity,
                .direction = direction,
                .mesh = -1,
                .transform = -1,
                .emission_tex = -1,
                .type = light_type::distant,
            };
            scene.lights.push_back(light);
        }
        scene.sky_light.intensity = glm::vec3{
            root_json.value<float>("/sky_light/intensity"_json_pointer, 0.0f),
            root_json.value<float>("/sky_light/intensity"_json_pointer, 0.0f),
            root_json.value<float>("/sky_light/intensity"_json_pointer, 0.0f),
        };
        std::string const& environment_map_file = root_json.value<std::string>(
            "/sky_light/environment_tex"_json_pointer, "");
        if (!environment_map_file.empty()) {
            scene.sky_light.environment_tex = get_texture(environment_map_file);
        }
        return std::make_tuple(options, camera, scene);
    } catch (std::exception& exp) {
        CHECK(false, "{}", exp.what());
    }
}
