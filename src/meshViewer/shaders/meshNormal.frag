#version 460

layout(location = 0) in vec3 worldNormal;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 mvp;
    mat4 normalMatrix;
    float opacity;
} ubo;

void main()
{
    outColor = vec4(worldNormal, clamp(ubo.opacity, 0.0, 1.0));
}