#version 440
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {  // warning: matches layout and padding of FloatImageViewerMaterial::uniforms!
    mat4 qt_Matrix;  // offset 0
    float qt_Opacity;  // offset 64
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

layout(binding = 1) uniform sampler2D tex;

void main()
{
    vec4 color = texture(tex, vTexCoord);

    color.rgb = pow(pow(max(color.rgb * vec3(gain), vec3(0.0, 0.0, 0.0)), vec3(1.0 / gamma)), vec3(1.0 / 2.2));

    fragColor.r = color[int(channelOrder[0])];
    fragColor.g = color[int(channelOrder[1])];
    fragColor.b = color[int(channelOrder[2])];
    fragColor.a = int(channelOrder[3]) == -1 ? 1.0 : color[int(channelOrder[3])];
    fragColor.a *= qt_Opacity;

    if (distance(vec2(vTexCoord.x * aspectRatio, vTexCoord.y), vec2(fisheyeCircleCoord.x * aspectRatio, fisheyeCircleCoord.y)) > fisheyeCircleRadius && fisheyeCircleRadius > 0.0)
    {
       fragColor.a *= 0.001;
    }
    fragColor.rgb *= fragColor.a;
}