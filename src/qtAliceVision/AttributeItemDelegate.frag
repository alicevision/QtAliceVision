#version 440
precision highp float;
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}
void main() {
    fragColor = vec4(hsv2rgb(vec3(qt_TexCoord0.x, 1.0, 1.0)), 1.0);
}