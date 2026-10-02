#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 mvp;
    // x = alpha
    vec4 params;
} ubo;

layout(location = 0) out vec4 vColor;

void main()
{
    vColor = vec4(inColor, ubo.params.x);
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
}
