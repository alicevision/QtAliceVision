#version 460

/**
 * @file meshPBR.frag
 * @brief MeshPBR mode: metallic-roughness PBR shading (GGX specular, Lambert diffuse) under a headlight.
 *
 * Base color = baseColor factor (linear, as in glTF) x base color texture. The base color texture is an sRGB GPU
 * texture, so sampling already returns linear values, filtered in linear space. Metallic = metallic factor x B,
 * roughness = roughness factor x G of the metallic-roughness texture (linear, glTF layout). Untextured slots are bound
 * to a white texture, so the factors alone apply. Lighting is computed in linear space and written back in sRGB,
 * like the colors of the other shading modes.
 */

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec3 worldPosition;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 mvp;
    mat4 normalMatrix;
    float opacity;
} ubo;

layout(std140, binding = 1) uniform MaterialBuffer {
    vec4 baseColor;
    vec4 pbr; // x: metallic factor, y: roughness factor, z: 1 when opaque (glTF alphaMode OPAQUE): base color alpha ignored
} material;

layout(binding = 2) uniform sampler2D baseColorMap;
layout(binding = 3) uniform sampler2D metallicRoughnessMap;

layout(std140, binding = 4) uniform PbrFrameBuffer {
    vec4 cameraPosition; // xyz: camera position in world space
} frame;

const float PI = 3.14159265359;

/** @brief Linear to sRGB, exact piecewise transfer function. */
vec3 linearToSrgb(vec3 c)
{
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), c));
}

/** @brief GGX / Trowbridge-Reitz normal distribution. */
float distributionGGX(float NdotH, float alpha)
{
    float a2 = alpha * alpha;
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

/**
 * @brief Separable Smith-Schlick approximation of the geometry term.
 * @param roughness Perceptual roughness.
 * @note Uses k = (roughness + 1)^2 / 8, the remapping for punctual lights (Karis, UE4).
 */
float geometrySmith(float NdotV, float NdotL, float roughness)
{
    float r = roughness + 1.0;
    float k = r * r / 8.0;
    float gv = NdotV / (NdotV * (1.0 - k) + k);
    float gl = NdotL / (NdotL * (1.0 - k) + k);
    return gv * gl;
}

/**
 * @brief Schlick Fresnel approximation.
 * @note The base of pow is clamped to [0, 1]: cosTheta can exceed 1.0 by rounding when V and H are nearly collinear
 *       (frequent with a headlight), and pow of a negative base is undefined in GLSL (NaN on some drivers).
 */
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec4 baseSample = texture(baseColorMap, uv) * material.baseColor;
    vec3 albedo = clamp(baseSample.rgb, 0.0, 1.0);

    vec4 mr = texture(metallicRoughnessMap, uv);
    float metallic = clamp(material.pbr.x * mr.b, 0.0, 1.0);
    float roughness = clamp(material.pbr.y * mr.g, 0.04, 1.0);
    float alpha = roughness * roughness;

    vec3 N = normalize(worldNormal);
    vec3 V = normalize(frame.cameraPosition.xyz - worldPosition);

    // Same light as MeshShaded: a fixed direction in view space (headlight), brought back to world space.
    // normalMatrix is the inverse-transpose of the view matrix, so its transpose maps view space to world space.
    vec3 L = normalize(transpose(mat3(ubo.normalMatrix)) * vec3(1.0, 2.0, 3.0));
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 1e-4);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(VdotH, F0);
    float D = distributionGGX(NdotH, alpha);
    float G = geometrySmith(NdotV, NdotL, roughness);

    vec3 specular = (D * G) * F / (4.0 * NdotV * max(NdotL, 1e-4));
    vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo / PI;

    /**
     * @note Stylised viewer lighting, not an energy-conserving model: a single headlight scaled so that a white
     *       Lambert surface facing it is ~1, plus a constant ambient term standing in for the missing environment
     *       lighting. Metals keep half of it so they do not turn black. MeshShaded adds 0.2 to its output, which is
     *       already in display (sRGB) space; this shader works in linear space, so the same level is
     *       srgbToLinear(0.2) ~= 0.033.
     */
    const float lightIntensity = PI;
    const float ambient = 0.033;
    vec3 color = (diffuse + specular) * lightIntensity * NdotL + ambient * albedo * (1.0 - 0.5 * metallic);

    float outAlpha = material.pbr.z > 0.5 ? 1.0 : baseSample.a;
    outColor = vec4(linearToSrgb(clamp(color, 0.0, 1.0)), outAlpha * clamp(ubo.opacity, 0.0, 1.0));
}
