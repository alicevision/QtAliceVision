#version 440

// Envelope triangle strip: (x, lower) and (x, upper) per sample.
layout(location = 0) in vec2 positionHi;  // data space, high parts
layout(location = 1) in vec2 positionLo;  // data space, low parts

layout(std140, binding = 0) uniform buf {  // warning: matches layout in CurveRenderer.cpp
    mat4 itemToClip;
    vec4 color;
    vec4 origin;
    vec2 scale;
    vec2 viewportSize;
    float halfWidth;
    float height;
};

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    // Same double-float emulation as CurveLine.vert
    vec2 d = (positionHi - origin.xy) + (positionLo - origin.zw);
    gl_Position = itemToClip * vec4(d.x * scale.x, height - d.y * scale.y, 0.0, 1.0);
}
