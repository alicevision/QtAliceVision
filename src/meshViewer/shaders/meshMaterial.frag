#version 460

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 mvp;
    mat4 normalMatrix;
    float opacity;
} ubo;

layout(std140, binding = 1) uniform MaterialBuffer {
    vec4 baseColor;
    vec4 params; // x: 1 when opaque (glTF alphaMode OPAQUE): base color alpha ignored
} material;

/** @brief sRGB GPU texture: sampling returns linear values, filtered and mipmapped in linear space. */
layout(binding = 2) uniform sampler2D baseColorMap;

/** @brief Linear to sRGB, exact piecewise transfer function. */
vec3 linearToSrgb(vec3 c)
{
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), c));
}

/**
 * @brief Unlit: photogrammetry textures already contain the scene lighting.
 *
 * Same convention as MeshPBR: the linear texture sample is multiplied with the linear base color factor, and the
 * product is encoded to sRGB.
 */
void main()
{
    vec4 color = texture(baseColorMap, inUV) * material.baseColor;
    float alpha = material.params.x > 0.5 ? 1.0 : color.a;
    outColor = vec4(linearToSrgb(clamp(color.rgb, 0.0, 1.0)), alpha * clamp(ubo.opacity, 0.0, 1.0));
}
