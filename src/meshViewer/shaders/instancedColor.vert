#version 460

layout(location = 0) in vec3 inPosition;       // per-vertex: local sphere position
layout(location = 1) in vec3 inColor;           // per-vertex: sphere color
layout(location = 2) in vec3 inInstanceOffset; // per-instance: world-space center

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 mvp;
    // x = sphere radius, y = fixed screen size flag, z = projection vertical scale, w = viewport height (pixels).
    vec4 params;
} ubo;

layout(location = 0) out vec3 outColor;

void main()
{
    outColor = inColor;

    float radius = ubo.params.x;
    if (ubo.params.y > 0.5)
    {
        // Radius is in pixels: convert to world units using the clip-space w of the center.
        // Valid for both perspective (w = depth) and orthographic (w = 1) projections.
        float w = (ubo.mvp * vec4(inInstanceOffset, 1.0)).w;
        radius *= 2.0 * w / (ubo.params.z * max(ubo.params.w, 1.0));
    }

    gl_Position = ubo.mvp * vec4(inPosition * radius + inInstanceOffset, 1.0);
}
