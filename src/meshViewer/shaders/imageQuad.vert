#version 460

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec2 vsNdc;

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 imageProjection;
} ubo;

void main()
{
    vsNdc = inPosition;
    gl_Position = ubo.imageProjection * vec4(inPosition, 0.0, 1.0);
}
