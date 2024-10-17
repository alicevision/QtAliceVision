#version 440

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix; //64
    vec4 color;
    float size;
};

void main()
{
    fragColor = color;
}