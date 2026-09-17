#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inBarycentric;

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 mvp;
    mat4 normalMatrix;
    float opacity;
} ubo;

layout(location = 0) out vec3 vBary;

void main()
{
    vBary = inBarycentric;
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
}
