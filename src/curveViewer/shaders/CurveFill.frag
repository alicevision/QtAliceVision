#version 440

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {  // warning: matches layout in CurveRenderer.cpp
    mat4 itemToClip;
    vec4 color;
    vec4 origin;
    vec2 scale;
    vec2 viewportSize;
    float halfWidth;
    float height;
};

void main()
{
    fragColor = color;
}
