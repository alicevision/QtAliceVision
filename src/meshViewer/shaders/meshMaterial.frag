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
} material;

layout(binding = 2) uniform sampler2D baseColorMap;

// Unlit: photogrammetry textures already contain the scene lighting.
void main()
{
    vec4 color = texture(baseColorMap, inUV) * material.baseColor;
    outColor = vec4(color.rgb, color.a * clamp(ubo.opacity, 0.0, 1.0));
}
