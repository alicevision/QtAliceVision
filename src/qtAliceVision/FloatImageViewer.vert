#version 440
layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texCoord;
layout(location = 0) out vec2 vTexCoord;
layout(std140, binding = 0) uniform buf {  // warning: matches layout and padding of FloatImageViewerMaterial::uniforms!
    mat4 qt_Matrix; // offset 0
    float qt_Opacity; // offset 64
    float _padding1;
    float _padding2;
    float _padding3;
    vec4 channelOrder;  // offset 80
    vec2 fisheyeCircleCoord;
    float gamma;
    float gain;
    float fisheyeCircleRadius;
    float aspectRatio;
};
out gl_PerVertex { vec4 gl_Position; };

void main()
{
    vTexCoord = texCoord;
    gl_Position = qt_Matrix * position;
}