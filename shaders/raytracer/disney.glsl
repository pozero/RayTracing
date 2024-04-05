vec3 to_local(const in vec3 X, 
              const in vec3 Y, 
              const in vec3 Z, 
              const in vec3 V) {
    return vec3(dot(V, X), dot(V, Y), dot(V, Z));
}

vec3 to_world(const in vec3 X, 
              const in vec3 Y, 
              const in vec3 Z, 
              const in vec3 V) {
    return V.x * X + V.y * Y + V.z * Z;
}

void tint_colors(const in surface_info_t surface_info, 
                out float F0, 
                out vec3 csheen, 
                out vec3 cspec0) {
    const float lum = luminance(surface_info.albedo);
    const vec3 ctint = lum > 0.0 ? surface_info.albedo / lum : vec3(1.0);
    F0 = (1 - surface_info.eta) / (1 + surface_info.eta);
    F0 *= F0;
    cspec0 = F0 * mix(vec3(1.0), ctint, surface_info.specular_tint);
    csheen = mix(vec3(1.0), ctint, surface_info.sheen_tint);
}

float schlick_weight(const in float u) {
    const float m = clamp(1.0 - u, 0.0, 1.0);
    const float m2 = m * m;
    return m2 * m2 * m;
}

vec4 eval_disney_diffuse(const in surface_info_t surface_info,
                         const in vec3 csheen,
                         const in vec3 V,
                         const in vec3 L,
                         const in vec3 H) {
    vec4 contrib = vec4(0.0);
    if (L.z <= 0.0) {
        return contrib;
    }
    const float L_H = dot(L, H);
    const float Rr = 2.0 * surface_info.roughness * L_H * L_H;
    // diffuse
    const float FL = schlick_weight(L.z);
    const float FV = schlick_weight(V.z);
    const float Fretro = Rr * (FL + FV + FL * FV * (Rr - 1.0));
    const float Fd = (1.0 - 0.5 * FL) * (1.0 - 0.5 * FV);
    // subsurface approx
    const float Fss90 = 0.5 * Rr;
    const float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    const float ss = 1.25 * (Fss * (1.0 / (L.z + V.z) - 0.5) + 0.5);
    // sheen
    const float FH = schlick_weight(L_H);
    const vec3 Fsheen = FH * surface_info.sheen * csheen;
    contrib.xyz = ONE_OVER_PI * surface_info.albedo * mix(Fd + Fretro, ss, surface_info.subsurface) + Fsheen;
    contrib.w = L.z * ONE_OVER_PI;
    return contrib;
}

float dielectric_fresnel(const in float cos_theta_i,
                         const in float eta) {
    const float sin_theta_t2 = eta * eta * (1.0f - cos_theta_i * cos_theta_i);
    // Total internal reflection
    if (sin_theta_t2 > 1.0)
        return 1.0;
    const float cos_theta_t = sqrt(max(1.0 - sin_theta_t2, 0.0));
    const float rs = (eta * cos_theta_t - cos_theta_i) / (eta * cos_theta_t + cos_theta_i);
    const float rp = (eta * cos_theta_i - cos_theta_t) / (eta * cos_theta_i + cos_theta_t);
    return 0.5 * (rs * rs + rp * rp);
}

float gtr2_aniso(const in float N_H, 
                 const in float H_X, 
                 const in float H_Y, 
                 const in float ax, 
                 const in float ay) {
    const float a = H_X / ax;
    const float b = H_Y / ay;
    const float c = a * a + b * b + N_H * N_H;
    return 1.0 / (PI * ax * ay * c * c);
}

float smith_g_aniso(const in float N_V, 
                    const in float V_X, 
                    const in float V_Y, 
                    const in float ax, 
                    const in float ay) {
    const float a = V_X * ax;
    const float b = V_Y * ay;
    const float c = N_V;
    return (2.0 * N_V) / (N_V + sqrt(a * a + b * b + c * c));
}

vec4 eval_microfacet_reflection(const in surface_info_t surface_info,
                                const in vec3 V,
                                const in vec3 L,
                                const in vec3 H,
                                const in vec3 F) {
    vec4 contrib = vec4(0.0);
    if (L.z <= 0.0) {
        return contrib;
    }
    const float D = gtr2_aniso(H.z, H.x, H.y, surface_info.ax, surface_info.ay);
    const float G1 = smith_g_aniso(abs(V.z), V.x, V.y, surface_info.ax, surface_info.ay);
    const float G2 = G1 * smith_g_aniso(abs(L.z), L.x, L.y, surface_info.ax, surface_info.ay);
    contrib.xyz = F * D * G2 / (4.0 * L.z * V.z);
    contrib.w = G1 * D / (4.0 * V.z);
    return contrib;
}

vec4 eval_microfacet_refraction(const in surface_info_t surface_info, 
                                const in vec3 V, 
                                const in vec3 L, 
                                const in vec3 H, 
                                const in vec3 F) {
    vec4 contrib = vec4(0.0);
    if (L.z >= 0.0) {
        return contrib;
    }
    const float L_H = dot(L, H);
    const float V_H = dot(V, H);
    const float D = gtr2_aniso(H.z, H.x, H.y, surface_info.ax, surface_info.ay);
    const float G1 = smith_g_aniso(abs(V.z), V.x, V.y, surface_info.ax, surface_info.ay);
    const float G2 = G1 * smith_g_aniso(abs(L.z), L.x, L.y, surface_info.ax, surface_info.ay);
    const float denom = L_H + V_H * surface_info.eta;
    const float denom2 = denom * denom;
    const float eta2 = surface_info.eta * surface_info.eta;
    const float jacobian = abs(L_H) / pad_above_zero(denom2);
    contrib.xyz = pow(surface_info.albedo, vec3(0.5)) * (vec3(1.0) - F) * D * G2 * abs(V_H) * jacobian * eta2 / 
        pad_above_zero(abs(L.z * V.z));
    contrib.w = G1 * max(0.0, V_H) * D * jacobian / pad_above_zero(V.z);
    return contrib;
}

float gtr1(const in float N_H, 
           const in float a) {
    if (a >= 1.0)
        return ONE_OVER_PI;
    const float a2 = a * a;
    const float t = 1.0 + (a2 - 1.0) * N_H * N_H;
    return (a2 - 1.0) / (PI * log(a2) * t);
}

float smith_g(const in float N_V, 
             const in float alpha) {
    const float a = alpha * alpha;
    const float b = N_V * N_V;
    return (2.0 * N_V) / (N_V + sqrt(a + b - a * b));
}

vec4 eval_clearcoat(const in surface_info_t surface_info, 
                    const in vec3 V, 
                    const in vec3 L, 
                    const in vec3 H) {
    vec4 contrib = vec4(0.0);
    if (L.z <= 0.0) {
        return contrib;
    }
    const float V_H = dot(V, H);
    const float F = mix(0.04, 1.0, schlick_weight(V_H));
    const float D = gtr1(H.z, surface_info.clearcoat_gloss);
    const float G = smith_g(L.z, 0.25) * smith_g(V.z, 0.25);
    const float jacobian = 1.0 / (4.0 * V_H);
    contrib.xyz = vec3(F) * D * G;
    contrib.w = D * H.z * jacobian;
    return contrib;
}

vec4 eval_disney(const in state_t state, 
                 const in surface_info_t surface_info,
                 in vec3 V, 
                 in vec3 L) {
    vec4 bsdf_pdf = vec4(0.0);
    vec3 T, B;
    const vec3 N = state.hit_normal;
    onb(N, T, B);
    V = to_local(T, B, N, -V);
    L = to_local(T, B, N, L);
    vec3 H = L.z > 0.0 ? normalize(L + V) : normalize(L + V * surface_info.eta);
    H = H.z < 0.0 ? -H : H;

    vec3 csheen, cspec0;
    float F0;
    tint_colors(surface_info, F0, csheen, cspec0);

    const float dielectric_wt = (1.0 - surface_info.metallic) * (1.0 - surface_info.spec_trans);
    const float metal_wt = surface_info.metallic;
    const float glass_wt = (1.0 - surface_info.metallic) * surface_info.spec_trans;
    const float schlick_wt = schlick_weight(V.z);
    float diffuse_pr = dielectric_wt * luminance(surface_info.albedo);
    float dielectric_pr = dielectric_wt * luminance(mix(cspec0, vec3(1.0), schlick_wt));
    float metal_pr = metal_wt * luminance(mix(surface_info.albedo, vec3(1.0), schlick_wt));
    float glass_pr = glass_wt;
    float clearcoat_pr = 0.25 * surface_info.clearcoat;
    const float one_pr_sum = 1.0 / (diffuse_pr + dielectric_pr + metal_pr + glass_pr + clearcoat_pr);
    diffuse_pr *= one_pr_sum;
    dielectric_pr *= one_pr_sum;
    metal_pr *= one_pr_sum;
    glass_pr *= one_pr_sum;
    clearcoat_pr *= one_pr_sum;

    const bool reflect = L.z * V.z > 0;
    const float abs_V_H = abs(dot(V, H));
    vec4 contrib = vec4(0.0);

    if (diffuse_pr > 0.0 && reflect) {
        contrib = eval_disney_diffuse(surface_info, csheen, V, L, H);
        bsdf_pdf.xyz += dielectric_wt * contrib.xyz;
        bsdf_pdf.w += diffuse_pr * contrib.w;
    }

    if (dielectric_pr > 0.0 && reflect) {
        const float F = (dielectric_fresnel(abs_V_H, 1.0 / surface_info.ior) - F0) / (1.0 - F0);
        contrib = eval_microfacet_reflection(surface_info, V, L, H, mix(cspec0, vec3(1.0), F));
        bsdf_pdf.xyz += dielectric_wt * contrib.xyz;
        bsdf_pdf.w += dielectric_pr * contrib.w;
    }

    if (metal_pr > 0.0 && reflect) {
        const vec3 F = mix(surface_info.albedo, vec3(1.0), schlick_weight(abs_V_H));
        contrib = eval_microfacet_reflection(surface_info, V, L, H, F);
        bsdf_pdf.xyz += metal_wt * contrib.xyz;
        bsdf_pdf.w += metal_pr * contrib.w;
    }

    if (glass_pr > 0.0) {
        const float F = dielectric_fresnel(abs_V_H, surface_info.eta);
        if (reflect) {
            contrib = eval_microfacet_reflection(surface_info, V, L, H, vec3(F));
            bsdf_pdf.xyz += glass_wt * contrib.xyz;
            bsdf_pdf.w += glass_pr * F * contrib.w;
        } else {
            contrib = eval_microfacet_refraction(surface_info, V, L, H, vec3(F));
            bsdf_pdf.xyz += glass_wt * contrib.xyz;
            bsdf_pdf.w += glass_pr * (1.0 - F) * contrib.w;
        }
    }

    if (clearcoat_pr > 0.0 && reflect) {
        contrib = eval_clearcoat(surface_info, V, L, H);
        bsdf_pdf.xyz += 0.25 * surface_info.clearcoat * contrib.xyz;
        bsdf_pdf.w += clearcoat_pr * contrib.w;
    }

    bsdf_pdf.xyz *= abs(L.z);
    return bsdf_pdf;
}

vec3 sample_disney(const in state_t state,
                   const in surface_info_t surface_info,
                   in vec3 V) {
    vec3 L;
    vec3 T, B;
    const vec3 N = state.hit_normal;
    onb(N, T, B);
    V = to_local(T, B, N, -V);

    vec3 csheen, cspec0;
    float F0;
    tint_colors(surface_info, F0, csheen, cspec0);

    const float dielectric_wt = (1.0 - surface_info.metallic) * (1.0 - surface_info.spec_trans);
    const float metal_wt = surface_info.metallic;
    const float glass_wt = (1.0 - surface_info.metallic) * surface_info.spec_trans;
    const float schlick_wt = schlick_weight(V.z);
    float diffuse_pr = dielectric_wt * luminance(surface_info.albedo);
    float dielectric_pr = dielectric_wt * luminance(mix(cspec0, vec3(1.0), schlick_wt));
    float metal_pr = metal_wt * luminance(mix(surface_info.albedo, vec3(1.0), schlick_wt));
    float glass_pr = glass_wt;
    float clearcoat_pr = 0.25 * surface_info.clearcoat;
    const float one_pr_sum = 1.0 / (diffuse_pr + dielectric_pr + metal_pr + glass_pr + clearcoat_pr);
    diffuse_pr *= one_pr_sum;
    dielectric_pr *= one_pr_sum;
    metal_pr *= one_pr_sum;
    glass_pr *= one_pr_sum;
    clearcoat_pr *= one_pr_sum;
    float cdf[5];
    cdf[0] = diffuse_pr;
    cdf[1] = cdf[0] + dielectric_pr;
    cdf[2] = cdf[1] + metal_pr;
    cdf[3] = cdf[2] + glass_pr;
    cdf[4] = cdf[3] + clearcoat_pr;

    const float pr = rand_01();
    if (pr < cdf[0]) {
        L = cosine_sample_hemisphere();
    } else if (pr < cdf[2]) {
        vec3 H = sample_ggx_vndf(V, surface_info.ax, surface_info.ay);
        if (H.z < 0.0) {
            H = -H;
        }
        L = normalize(reflect(-V, H));
    } else if (pr < cdf[3]) {
        vec3 H = sample_ggx_vndf(V, surface_info.ax, surface_info.ay);
        if (H.z < 0.0) {
            H = -H;
        }
        const float F = dielectric_fresnel(abs(dot(V, H)), surface_info.eta);
        const float pr_f = rand_01();
        if (pr_f < F) {
            L = normalize(reflect(-V, H));
        } else {
            L = normalize(refract(-V, H, surface_info.eta));
        }
    } else {
        vec3 H = sample_gtr1(surface_info.clearcoat_gloss);
        if (H.z < 0.0) {
            H = -H;
        }
        L = normalize(reflect(-V, H));
    }
    L = to_world(T, B, N, L);
    return L;
}
