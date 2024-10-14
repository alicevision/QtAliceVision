#version 440

layout(location = 0) in vec4 vertexCoord;

layout(std140, binding = 0) uniform buf {
    mat4 matrix; //64
    vec4 color;
    float size;
};

void main()
{
    gl_Position = matrix * vertexCoord;
    gl_PointSize = size;
}