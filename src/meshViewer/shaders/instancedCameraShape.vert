#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in mat4 inTransform;

layout(location = 6) in float inScale;

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 viewProjection;
} ubo;

layout(location = 0) out vec3 outColor;

void main()
{
    float scale = inScale;

    outColor = inColor;
    gl_Position = ubo.viewProjection * inTransform * vec4(inPosition * scale, 1.0);
}
