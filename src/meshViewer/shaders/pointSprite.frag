#version 460

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inQuadCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    if (dot(inQuadCoord, inQuadCoord) > 1.0)
    {
        discard;
    }

    outColor = vec4(inColor, 1.0);
}