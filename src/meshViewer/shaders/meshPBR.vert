#version 460

/**
 * @file meshPBR.vert
 * @brief MeshPBR mode: forwards UV, world-space normal and position to the PBR fragment shader.
 */

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 mvp;
    mat4 normalMatrix;
    float opacity;
} ubo;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec3 worldPosition;

void main()
{
    // The mesh model matrix is identity: vertex attributes are already in world space.
    uv = inUV;
    worldNormal = inNormal;
    worldPosition = inPosition;
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
}
