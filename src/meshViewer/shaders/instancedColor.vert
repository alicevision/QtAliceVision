#version 460

layout(location = 0) in vec3 inPosition;       // per-vertex: local sphere position
layout(location = 1) in vec3 inColor;           // per-vertex: sphere color
layout(location = 2) in vec3 inInstanceOffset; // per-instance: world-space center

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 mvp;
} ubo;

layout(location = 0) out vec3 outColor;

void main()
{
    outColor = inColor;
    gl_Position = ubo.mvp * vec4(inPosition + inInstanceOffset, 1.0);
}
