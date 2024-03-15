#version 460

#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_nonuniform_qualifier: require

layout(constant_id = 0) const uint POINT_LIGHT_COUNT = 1;

layout(constant_id = 1) const uint MAX_ROUGHNESS_MIP = 1;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;
layout (location = 3) in flat uint in_material;

layout (location = 0) out vec4 out_frag;

#include "../common/to_ldr.glsl"

#define PI 3.1415926535

struct cook_torrance_material_t {
    vec4 albedo;
    float metallic;
    float roughness;
    float ao;
};

struct point_light {
    vec4 position;
    vec4 color;
};

float distribution_ggx(const in vec3 normal,
                       const in vec3 half_vec,
                       const in float roughness);

float direct_lightiing_geometry_schlick_ggx(const in float dot_product,
                                            const in float roughness);

float direct_lightiing_geometry_smith(const in vec3 normal,
                                      const in vec3 view,
                                      const in vec3 light,
                                      const in float roughness);

vec3 fresnel_schlick(const in vec3 half_vec,
                     const in vec3 view,
                     const in vec3 f0);

vec3 fresnel_schlick_roughness(const in vec3 normal,
                               const in vec3 view,
                               const in vec3 f0, 
                               const float roughness);

vec3 calculate_f0(const in cook_torrance_material_t material);

layout(push_constant, std430) uniform push_constants {
    layout(offset = 64) vec4 camera_position;
};

layout(std430, set = 1, binding = 0) readonly buffer triangle_materials {
    cook_torrance_material_t materials[];
};

layout(std430, set = 1, binding = 1) readonly buffer point_lights {
    point_light lights[POINT_LIGHT_COUNT];
};

layout(set = 1, binding = 2) uniform samplerCube lambertian_diffuse_irradiance_map;

layout(set = 1, binding = 3) uniform samplerCube prefiltered_environment_map;

layout(set = 1, binding = 4) uniform sampler2D brdf_lut;

void main() {
    const cook_torrance_material_t material = materials[nonuniformEXT(in_material)];
    const vec3 normal = normalize(in_normal);
    const vec3 view = normalize(camera_position.xyz - in_position);
    const float n_dot_v = max(dot(normal, view), 0.0);
    const vec3 f0 = calculate_f0(material);
    vec3 lo = vec3(0.0);
    for (uint i = 0; i < POINT_LIGHT_COUNT; ++ i) {
        const vec3 light_to_frag = lights[i].position.xyz - in_position;
        const vec3 light_vec = normalize(light_to_frag);
        const vec3 half_vec = normalize(view + light_vec);
        const float light_distance = length(light_to_frag);
        const float attenuation = 1.0 / (light_distance * light_distance);
        const vec3 radiance = lights[i].color.xyz * attenuation;
        const float n_dot_l = max(dot(normal, light_vec), 0.0);
        // specular
        const float normal_distribution = distribution_ggx(normal, half_vec, material.roughness);
        const float geometry = direct_lightiing_geometry_smith(normal, view, light_vec, material.roughness);
        const vec3 fresnel = fresnel_schlick(half_vec, view, f0);
        const vec3 specular_numerator = normal_distribution * geometry * fresnel;
        const float specular_denominator = 4.0 * n_dot_v * n_dot_l + 0.0001;
        const vec3 specular = specular_numerator / specular_denominator;
        // diffuse
        const vec3 k_diffuse = (1.0 - material.metallic) * (vec3(1.0) - fresnel);
        const vec3 diffuse = (k_diffuse * material.albedo.xyz) / PI;
        lo += (diffuse + specular) * radiance * n_dot_l;
    }
    const vec3 relfect_vec = reflect(-view, normal);
    const vec3 k_specular_ambient = fresnel_schlick_roughness(normal, view, f0, material.roughness);
    const vec3 k_diffuse_ambient = (1.0 - material.metallic) * (1.0 - k_specular_ambient);
    const vec3 lambertian_diffuse_irradiance = texture(lambertian_diffuse_irradiance_map, normal).rgb;
    const vec3 diffuse_ambient = lambertian_diffuse_irradiance * material.albedo.xyz;
    const vec3 prefiltered_environment = textureLod(prefiltered_environment_map, relfect_vec, 
        material.roughness * MAX_ROUGHNESS_MIP).rgb;
    const vec2 brdf_environment = 
        texture(brdf_lut, vec2(max(dot(normal, view), 0.0), material.roughness)).rg;
    const vec3 specular_ambient = 
        prefiltered_environment * (brdf_environment.x * k_specular_ambient + brdf_environment.y);
    const vec3 ambient = (k_diffuse_ambient * diffuse_ambient + specular_ambient) * material.ao;
    const vec3 linear_color = ambient + lo;
    const vec3 corrected_color = gamma_correct(tone_mapping(linear_color));
    out_frag = vec4(corrected_color, 1.0);
    // out_frag = vec4(vec2(isnan(brdf_environment)), 0.0, 1.0);
}

// \frac{\alpha^2}{\pi((n\cdot h)^2(\alpha^2-1)+1)^2}
float distribution_ggx(const in vec3 normal,
                       const in vec3 half_vec,
                       const in float roughness) {
    // making the result more plausible
    const float a = roughness * roughness;
    const float a_squared = a * a;
    const float n_dot_h = max(dot(normal, half_vec), 0.0);
    const float n_dot_h_squared = n_dot_h * n_dot_h;
    const float numerator = a_squared;
    float denominator = n_dot_h_squared * (a_squared - 1.0) + 1.0;
    denominator = PI * denominator * denominator;
    return numerator / denominator;
}

// \frac{n\cdot v}{(n\cdot v)(1-k)+k}
float direct_lightiing_geometry_schlick_ggx(const in float dot_product,
                                            const in float roughness) {
    float k = roughness + 1.0;
    k = (k * k) / 8.0;
    const float numerator = dot_product;
    const float denominator = dot_product * (1.0 - k) + k;
    return numerator / denominator;
}

float direct_lightiing_geometry_smith(const in vec3 normal,
                                      const in vec3 view,
                                      const in vec3 light,
                                      const in float roughness) {
    const float n_dot_v = max(dot(normal, view), 0.0);
    const float n_dot_l = max(dot(normal, light), 0.0);
    const float geometry_masking = direct_lightiing_geometry_schlick_ggx(n_dot_v, roughness);
    const float geometry_shadowing = direct_lightiing_geometry_schlick_ggx(n_dot_l, roughness);
    return geometry_masking * geometry_shadowing;
}

// F_0+(1-F_0)(1-(h\cdot v))^5
vec3 fresnel_schlick(const in vec3 half_vec,
                     const in vec3 view,
                     const in vec3 f0) {
    const float h_dot_v = clamp(dot(half_vec, view), 0.0, 1.0);
    return f0 + (1.0 - f0) * pow(1.0 - h_dot_v, 5.0);
}


vec3 fresnel_schlick_roughness(const in vec3 normal,
                               const in vec3 view,
                               const in vec3 f0, 
                               const float roughness) {
    const float n_dot_v = clamp(dot(normal, view), 0.0, 1.0);
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - n_dot_v, 5.0);
}

vec3 calculate_f0(const in cook_torrance_material_t material) {
    const vec3 non_mental_f0 = vec3(0.04);
    return mix(non_mental_f0, material.albedo.xyz, material.metallic);
}
