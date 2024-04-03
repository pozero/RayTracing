vec3 to_local(const in vec3 x,
              const in vec3 y,
              const in vec3 z,
              const in vec3 vec) {
    return vec3(dot(vec, x), dot(vec, y), dot(vec, z));
}

vec3 to_world(const in vec3 x,
              const in vec3 y,
              const in vec3 z,
              const in vec3 vec) {
    return vec.x * x + vec.y * y + vec.z * z;
}

float schlick_weight(const in float u) {
    const float m = clamp(1.0 - u, 0.0, 1.0);
    return pow5(m);
}

float dielectric_fresnel(const in float cos_theta,
                         const in float eta) {
    float sin_theta_tsq = eta * eta * (1.0f - cos_theta * cos_theta);
    // total internal reflection
    if (sin_theta_tsq > 1.0)
        return 1.0;
    float cos_theta_tran = sqrt(max(1.0 - sin_theta_tsq, 0.0));
    float rs = (eta * cos_theta_tran - cos_theta) / (eta * cos_theta_tran + cos_theta);
    float rp = (eta * cos_theta - cos_theta_tran) / (eta * cos_theta + cos_theta_tran);
    return 0.5 * (rs * rs + rp * rp);
}

float gtr2_aniso(const in float n_dot_wm, 
                 const in float wm_dot_x, 
                 const in float wm_dot_y, 
                 const in float ax, 
                 const in float ay) {
    const float a = wm_dot_x / ax;
    const float b = wm_dot_y / ay;
    const float c = square(a) + square(b) + square(n_dot_wm);
    return 1.0 / (PI * ax * ay * square(c));
}


float smith_g_aniso(const in float n_dot_wi, 
                  const in float wo_dot_x, 
                  const in float wo_dot_y, 
                  const in float ax, 
                  const in float ay) {
    const float a = wo_dot_x * ax;
    const float b = wo_dot_y * ay;
    const float c = n_dot_wi;
    return (2.0 * n_dot_wi) / (n_dot_wi + sqrt(square(a) + square(b) + square(c)));
}

float gtr1(const in float n_dot_wm, 
           const in float a) {
    if (a >= 1.0)
        return ONE_OVER_PI;
    const float a2 = square(a);
    const float t = 1.0 + (a2 - 1.0) * n_dot_wm * n_dot_wm;
    return (a2 - 1.0) / (PI * log(a2) * t);
}

float smith_g(const in float dot_product, 
              const in float alpha) {
    float a = square(alpha);
    float b = square(dot_product);
    return (2.0 * dot_product) / (dot_product + sqrt(a + b - a * b));
}

bool microsurface_dist_calculated;
float disney_bsdf_d;
float disney_bsdf_g1;
float disney_bsdf_g2;

void calculate_microsurface_dist(const in vec3 wo,
                                 const in vec3 wm,
                                 const in vec3 wi,
                                 const in surface_info_t surface_info) {
    if (!microsurface_dist_calculated) {
        disney_bsdf_d = gtr2_aniso(wm.z, wm.x, wm.y, surface_info.ax, surface_info.ay);
        disney_bsdf_g1 = smith_g_aniso(abs(wo.z), wo.x, wo.y, surface_info.ax, surface_info.ay);
        disney_bsdf_g2 = smith_g_aniso(abs(wi.z), wi.x, wi.y, surface_info.ax, surface_info.ay);
        microsurface_dist_calculated = true;
    }
}

vec4 evaluate_disney_bsdf(const in state_t state,
                          const in surface_info_t surface_info,
                          const in vec3 view_vec,
                          const in vec3 light_vec) {
    microsurface_dist_calculated = false;
    vec4 bsdf_pdf = vec4(0.0);
    const vec3 n = get_front_face_normal(state);
    const vec3 wo = to_local(state.hit_tangent, state.hit_bitangent, n, -view_vec);
    const vec3 wi = to_local(state.hit_tangent, state.hit_bitangent, n, light_vec);
    const vec3 front_wm = 
        wi.z > 0.0 ? normalize(wo + wi) : normalize(wo * surface_info.eta + wi);
    const vec3 wm = front_wm.z < 0.0 ? -front_h : front_wm;

    const float albedo_lumiannce = luminance(surface_info.albedo);
    const vec3 ctint = albedo_lumiannce > 0 ? 
        surface_info.albedo / albedo_lumiannce :
        vec3(1.0);
    const float f0 = square((1 - surface_info.eta) / (1 + surface_info.eta));
    const vec3 cspec0 = f0 * mix(vec3(1.0), ctint, surface_info.specular_tint);
    const vec3 csheen = mix(vec3(1.0), ctint, surface_info.sheen_tint);

    // weight
    const float dielectric_w = 
        (1 - surface_info.metallic) * (1 - surface_info.spec_trans);
    const float metal_w = surface_info.metallic;
    const float glass_w = (1 - surface_info.metallic) * surface_info.spec_trans;
    const float schlick_w = schlick_weight(wo.z);
    // probability
    float diffuse_p = dielectric_w * luminance(surface_info.albedo);
    float dielectric_p = dielectric_w * luminance(mix(cspec0, vec3(1.0), schlick_w));
    float metal_p = metal_w * luminance(mix(surface_info.albedo, vec3(1.0), schlick_w));
    float glass_p = glass_w;
    float clearcoat_p = 0.25 * surface_info.clearcoat;
    const float one_over_sum_p = 
        1.0 / (diffuse_p + dielectric_p + metal_p + glass_p + clearcoat_p);
    diffuse_p *= one_over_sum_p;
    dielectric_p *= one_over_sum_p;
    metal_p *= one_over_sum_p;
    glass_p *= one_over_sum_p;
    clearcoat_p *= one_over_sum_p;

    const bool reflect = wo.z * wi.z > 0;
    const bool light_above_surface = wi.z > 0;
    const float wo_dot_wm = dot(wo, wm)
    const float wi_dot_wm = dot(wi, wm);
    if (diffuse_p > 0 && light_above_surface && reflect) {
        // diffuse
        const float fl = schlick_weight(wi.z);
        const float fv = schlick_weight(wo.z);
        const float rr = 2 * surface_info.roughness * square(wi_dot_wm);
        const float fd = (1 - 0.5 * fl) * (1 - 0.5 * fv);
        const float fretro = rr * (fl + fv + fl * fv * (rr - 1));
        // subsurface approximation
        const float fss90 = 0.5 * rr;
        const float fss = mix(1.0, fss90, fl) * mix(1.0, fss90, fv);
        const float ss = 1.25 * (fss * (1 / (wi.z + wo.z) - 0.5) + 0.5);
        // sheen
        const float fh = schlick_weight(wi_dot_wm);
        const vec3 fsheen = fh * surface_info.sheen * csheen;
        bsdf_pdf.xyz += dielectric_w * 
            (ONE_OVER_PI * surface_info.albedo * 
                mix(fd + fretro, ss, surface_info.subsurface) + fsheen);
        // cos-weighted hemisphere sampling
        bsdf_pdf.w += diffuse_p * (ONE_OVER_PI * wi.z);
    }
    if (dielectric_p > 0 && light_above_surface && reflect) {
        const float f = (dielectric_fresnel(abs(wo_dot_wm), 1 / surface_info.ior) - f0) /
            (1 - f0);
        const float f_mix = mix(cspec0, vec3(1.0), f);
        calculate_microsurface_dist(wo, wm, wi, surface_info);
        bsdf_pdf.xyz += dielectric_w * 
            (f_mix * disney_bsdf_d * disney_bsdf_g1 * disney_bsdf_g2) / 
            (4.0 * wo.z * wi.z);
        bsdf_pdf.w += dielectric_p * (disney_bsdf_g1 * disney_bsdf_d) / (4.0 * wo.z);
    }
    if (metal_p > 0 && light_above_surface && reflect) {
        const vec3 f = mix(surface_info.albedo, vec3(1.0), schlick_weight(wo_dot_wm));
        calculate_microsurface_dist(wo, wm, wi, surface_info);
        bsdf_pdf.xyz += metal_w * 
            (f * disney_bsdf_d * disney_bsdf_g1 * disney_bsdf_g2) / (4.0 * wo.z * wi.z);
        bsdf_pdf.w += metal_p * (disney_bsdf_g1 * disney_bsdf_d) / (4.0 * wo.z);
    }
    if (glass_p > 0) {
        const float f = dielectric_fresnel(wo_dot_wm, surface_info.eta);
        if (reflect && light_above_surface) {
            calculate_microsurface_dist(wo, wm, wi, surface_info);
            bsdf_pdf.xyz += glass_w *
                (vec3(f) * disney_bsdf_d * disney_bsdf_g1 * disney_bsdf_g2) /
                (4.0 * wo.z * wi.z);
            bsdf_pdf.w += f * glass_p * 
                ((disney_bsdf_d * disney_bsdf_g1) / (4.0 * wo.z));
        } else if (!light_above_surface) {
            calculate_microsurface_dist(wo, wm, wi, surface_info);
            const float demoninator = square(wi_dot_wm + surface_info.eta * wo_dot_wm);
            const float jacobian = abs(wi_dot_wm) / demoninator;
            bsdf_pdf.xyz += glass_w *
                pow(surface_info.albedo, vec3(0.5)) * (1 - f) * disney_bsdf_d * 
                disney_bsdf_g1 * disney_bsdf_g2 * abs(wo_dot_wm) * jacobian * 
                square(surface_info.eta) / abs(wi.z * wo.z);
            bsdf_pdf.w += (1 - f) * glass_p * 
                (max(0, wo_dot_wm) * disney_bsdf_g1 * disney_bsdf_d * jacobian / wo.z);
        }
    }
    if (clearcoat_p > 0 && light_above_surface && reflect) {
        const float f = mix(0.04, 1.0, schlick_weight(wo_dot_wm));
        const float d = gtr1(wm.z, surface_info.clearcoat_gloss);
        const float g = smith_g(wo.z, 0.25) * smith_g(wi.z, 0.25);
        const float jacobian = 1.0 / (4.0 * wo_dot_wm);
        bsdf_pdf.xyz += 0.25 * surface_info.clearcoat * (vec3(f) * d * g);
        bsdf_pdf.w += clearcoat_p * (d * wm.z * jacobian);
    }
    return bsdf_pdf;
}

vec3 sample_disney_bsdf(const in state_t state,
                        const in surface_info_t surface_info,
                        const in vec3 view_vec) {
    vec3 wi = vec4(0.0);
    const vec3 n = get_front_face_normal(state);
    const vec3 wo = to_local(state.hit_tangent, state.hit_bitangent, n, -view_vec);

    const float albedo_lumiannce = luminance(surface_info.albedo);
    const vec3 ctint = albedo_lumiannce > 0 ? 
        surface_info.albedo / albedo_lumiannce :
        vec3(1.0);
    const float f0 = square((1 - surface_info.eta) / (1 + surface_info.eta));
    const vec3 cspec0 = f0 * mix(vec3(1.0), ctint, surface_info.specular_tint);

    // weight
    const float dielectric_w = 
        (1 - surface_info.metallic) * (1 - surface_info.spec_trans);
    const float metal_w = surface_info.metallic;
    const float glass_w = (1 - surface_info.metallic) * surface_info.spec_trans;
    const float schlick_w = schlick_weight(wo.z);
    // unnormalized probability
    const float diffuse_p = dielectric_w * luminance(surface_info.albedo);
    const float dielectric_p = dielectric_w * luminance(mix(cspec0, vec3(1.0), schlick_w));
    const float metal_p = metal_w * luminance(mix(surface_info.albedo, vec3(1.0), schlick_w));
    const float glass_p = glass_w;
    const float clearcoat_p = 0.25 * surface_info.clearcoat;
    const float sum_p = diffuse_p + dielectric_p + metal_p + glass_p + clearcoat_p;
    const float cdf[5] = float[5](
        diffuse_p,
        diffuse_p + dielectric_p,
        diffuse_p + dielectric_p + metal_p,
        diffuse_p + dielectric_p + metal_p + glass_p,
        sum_p
    );
    const float r = sum_p * rand_01();
    if (r < cdf[0]) { // diffuse
        wi = cosine_sampling_sphere();
    } else if (r < cdf[2]) { // dielectric + metallic reflection
        vec3 wm = sample_ggx_visible_ndf(wo, surface_info.ax, surface_info.ay);
        if (wm.z < 0.0) {
            wm = -wm;
        }
        wi = normalize(reflect(-wo, wm));
    } else if (r < cdf[3]) {
        vec3 wm = sample_ggx_visible_ndf(wo, surface_info.ax, surface_info.ay);
        if (wm.z < 0.0) {
            wm = -wm;
        }
        const float f = dielectric_fresnel(abs(dot(wo, wm)), surface_info.eta);
        const float scaled_r = (r - cdf[2]) / (cdf[3] - cdf[2]);
        if (scaled_r < f) {
            wi = normalize(reflect(-wo, wm));
        } else {
            wi = normalize(refract(-wo, wm, surface_info.eta));
        }
    } else {
        vec3 wm = sample_gtr1(surface_info.clearcoat_gloss);
        if (wm.z < 0.0) {
            wm = -wm;
        }
        wi = normalize(reflect(-wo, wm));
    }
    return wi;
}
