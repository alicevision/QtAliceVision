#version 440

// Each segment [a, b] is a quad of 4 vertices, extruded in screen space
// so that the line width stays constant whatever the zoom.
layout(location = 0) in float corner;     // 0: (a, -1), 1: (a, +1), 2: (b, -1), 3: (b, +1)
layout(location = 1) in vec4 segmentHi;   // a.xy, b.xy in data space, high parts
layout(location = 2) in vec4 segmentLo;   // a.xy, b.xy in data space, low parts

layout(location = 0) out float vDistance;

layout(std140, binding = 0) uniform buf {  // warning: matches layout in CurveRenderer.cpp
    mat4 itemToClip;    // offset 0
    vec4 color;         // offset 64, premultiplied
    vec4 origin;        // offset 80, view origin (xMin, yMin): high parts in xy, low parts in zw
    vec2 scale;         // offset 96, data to item scale
    vec2 viewportSize;  // offset 104, device pixels
    float halfWidth;    // offset 112, device pixels
    float height;       // offset 116, item height
};

out gl_PerVertex { vec4 gl_Position; };

// Data to item coordinates. The origin is subtracted on high and low parts separately
// (double-float emulation): close to the view the result is small and exact.
vec4 toClip(vec2 hi, vec2 lo)
{
    vec2 d = (hi - origin.xy) + (lo - origin.zw);
    return itemToClip * vec4(d.x * scale.x, height - d.y * scale.y, 0.0, 1.0);
}

void main()
{
    vec4 clipA = toClip(segmentHi.xy, segmentLo.xy);
    vec4 clipB = toClip(segmentHi.zw, segmentLo.zw);

    vec2 halfViewport = 0.5 * viewportSize;
    vec2 pixelA = clipA.xy / clipA.w * halfViewport;
    vec2 pixelB = clipB.xy / clipB.w * halfViewport;

    vec2 delta = pixelB - pixelA;
    float len = length(delta);
    vec2 dir = len > 1e-6 ? delta / len : vec2(1.0, 0.0);
    vec2 normal = vec2(-dir.y, dir.x);

    float atB = corner >= 1.5 ? 1.0 : 0.0;
    float side = mod(corner, 2.0) >= 0.5 ? 1.0 : -1.0;

    // 1 extra pixel for antialiasing on each side, extend along the segment to cover joints
    float extent = halfWidth + 1.0;
    vec2 pixel = mix(pixelA, pixelB, atB) + normal * side * extent + dir * (2.0 * atB - 1.0) * halfWidth;

    vDistance = side * extent;
    gl_Position = vec4(pixel / halfViewport, clipA.z / clipA.w, 1.0);
}
