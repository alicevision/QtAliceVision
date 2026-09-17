#version 460

// Per-vertex: shared unit quad corner, in [-1, 1].
layout(location = 0) in vec2 inQuadOffset;
// Per-instance: point position and color.
layout(location = 1) in vec3 inInstancePosition;
layout(location = 2) in vec3 inInstanceColor;

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 mvp;
    // x = sprite diameter in pixels, y = viewport width, z = viewport height.
    vec4 spriteInfo;
} ubo;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outQuadCoord;

void main()
{
    vec4 centerClip = ubo.mvp * vec4(inInstancePosition, 1.0);

    float pixelSize = clamp(ubo.spriteInfo.x, 1.0, 256.0);
    vec2 viewportSize = max(ubo.spriteInfo.yz, vec2(1.0));

    // Expand the quad in clip space so its screen-space size stays constant
    // (independent of depth) once the perspective divide happens. This works
    // identically across all RHI backends, unlike gl_PointSize which some
    // backends (e.g. Direct3D) do not honor.
    // NDC spans [-1, 1] (2 units) across viewportSize pixels, hence the 2.0 factor.
    vec2 ndcOffset = inQuadOffset * (2.0 * pixelSize / viewportSize);

    vec4 clipPosition = centerClip;
    clipPosition.xy += ndcOffset * centerClip.w;

    gl_Position = clipPosition;
    outColor = inInstanceColor;
    outQuadCoord = inQuadOffset;
}