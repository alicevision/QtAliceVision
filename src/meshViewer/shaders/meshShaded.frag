#version 460

layout(location = 0) in vec3 inNormal;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 mvp;
    mat4 normalMatrix;
    float opacity;
} ubo;

void main()
{
    vec3 lightDir = normalize(vec3(1.0, 2.0, 3.0));
    float diffuse = max(dot(inNormal, lightDir), 0.0);
    float ambient = 0.2;
    float intensity = ambient + diffuse;
    outColor = vec4(vec3(0.7, 0.8, 0.9) * intensity, clamp(ubo.opacity, 0.0, 1.0));
}