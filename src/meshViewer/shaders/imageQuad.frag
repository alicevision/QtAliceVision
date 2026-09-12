#version 460

layout(location = 0) in vec2 vsNdc;

layout(binding = 1) uniform sampler2D imageSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    // The quad geometry is already scaled to the image's on-screen footprint by
    // the vertex shader's projection matrix, so vsNdc maps directly to UV space.
    vec2 uv = vsNdc * vec2(0.5, -0.5) + 0.5;
    outColor = texture(imageSampler, uv);
}
