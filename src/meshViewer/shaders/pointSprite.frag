#version 460

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 quadCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    if (dot(quadCoord, quadCoord) > 1.0)
    {
        discard;
    }

    outColor = vec4(color, 1.0);
}