#version 450
precision highp float;
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    lowp vec2 uvCenterOffset;
};
layout(binding = 1) uniform sampler2D src;

void main()
{
    vec2 xy = qt_TexCoord0 + uvCenterOffset;
    fragColor = texture(src, xy);
    fragColor.rgb *= qt_Opacity;
    fragColor.a = qt_Opacity;
    // remove undistortion black pixels
    fragColor.a *= step(0.001, fragColor.r + fragColor.g + fragColor.b);
}