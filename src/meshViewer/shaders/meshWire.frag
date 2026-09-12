#version 460

layout(location = 0) in vec3 vBary;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform UniformBuffer {
    mat4 mvp;
    mat4 normalMatrix;
    float opacity;
} ubo;

void main()
{
    // Screen-space partial derivatives give the barycentric rate-of-change
    // per pixel, which we use as the AA transition width.
    vec3 d = fwidth(vBary);

    // Smooth transition over ~1.5 pixels from edge to interior.
    vec3 a3 = smoothstep(vec3(0.0), d * 1.5, vBary);
    float edgeFactor = min(min(a3.x, a3.y), a3.z);
    float alpha = 1.0 - edgeFactor;

    // Discard fully interior fragments so they do not consume fill rate
    // and do not overwrite the depth buffer unnecessarily.
    if (alpha < 0.01)
    {
        discard;
    }

    outColor = vec4(1.0, 1.0, 1.0, alpha * clamp(ubo.opacity, 0.0, 1.0));
}
