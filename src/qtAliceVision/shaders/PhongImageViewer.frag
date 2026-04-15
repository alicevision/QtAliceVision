#version 440
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {  

    // warning: matches layout and padding of PhongImageViewerMaterial::uniforms

    mat4 qt_Matrix;    // offset 0
    float qt_Opacity;  // offset 64
    float _padding1;
    float _padding2;
    float _padding3;
    vec4 channelOrder; // offset 80
    vec4 baseColor;
    vec3 lightDirection;
    float textureOpacity;
    float gamma;
    float gain;
    float ka;
    float kd;
    float ks;
    float shininess;
};

layout(binding = 1) uniform sampler2D sourceSampler;
layout(binding = 2) uniform sampler2D normalSampler;

void main()
{
    vec4 diffuseColor;
    vec4 tColor = texture(sourceSampler, vTexCoord);
    vec3 normal = texture(normalSampler, vTexCoord).rgb;

    // apply gamma and gain on linear source texture
    tColor.rgb = pow((tColor.rgb * vec3(gain)), vec3(1.0/gamma));

    // apply channel mode
    diffuseColor.r = tColor[int(channelOrder[0])];
    diffuseColor.g = tColor[int(channelOrder[1])];
    diffuseColor.b = tColor[int(channelOrder[2])];
    diffuseColor.a = int(channelOrder[3]) == -1 ? 1.0 : tColor[int(channelOrder[3])];

    // apply texture opacity
    diffuseColor *= textureOpacity;
    diffuseColor += baseColor * (1.0 - textureOpacity);
    diffuseColor.a *= qt_Opacity;

    // Blinn-Phong shading
    float lambertian = max(dot(lightDirection, normal), 0.0);
    float specular = 0.0;

    if (lambertian > 0.0) 
    {
        vec3 viewDir = vec3(0, 0, -1);
        vec3 halfDir = normalize(lightDirection + viewDir);
        float specularAngle = max(dot(halfDir, normal), 0.0);
        specular = pow(specularAngle, shininess);
    }

    vec3 outLinear = vec3(ka, ka, ka) + kd * diffuseColor.rgb * lambertian + vec3(ks, ks, ks) * specular;
    vec3 outGammaCorrected = pow(outLinear, vec3(1.0 / 2.2)); // assume the monitor is calibrated to the sRGB colorspace
    fragColor = vec4(outGammaCorrected, diffuseColor.a);
}