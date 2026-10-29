#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 mvp;
} ubo;

layout(location = 0) out vec3 outColor;

void main()
{
    outColor = inColor;
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
}
