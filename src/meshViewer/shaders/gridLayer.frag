#version 460

// Full-screen procedural grid shader.
//
// The vertex stage renders a screen-filling quad. This fragment stage then:
// 1. Reconstructs the world-space view ray for the current pixel.
// 2. Intersects that ray against the ground plane y = 0.
// 3. Evaluates analytic grid coverage in world space using screen-space
//    derivatives for anti-aliasing.
// 4. Fades minor grid lines when a minor cell becomes too small in pixels.
//
// The important detail for the minor grid is that the fade is evaluated per
// axis. With an inclined camera, one grid direction is often much more
// foreshortened than the other. A single shared fade factor would let the worst
// axis hide both line families too early, which is the behavior this shader is
// avoiding.

layout(std140, binding = 0) uniform GridUniformBuffer {
    // Inverse of projection * view. Used to unproject screen positions back
    // into world space directly in the fragment shader.
    mat4 inverseViewProjection;
    // xy = viewport size in pixels. z/w are currently unused padding slots.
    vec4 viewportSize;
    // x = major spacing, y = minor spacing, z = major line width in pixels,
    // w = minor line width in pixels.
    vec4 spacingAndWidth;
    // x = minor opacity, y = fade start in pixels, z = fade end in pixels.
    // w is currently unused padding.
    vec4 minorGridParams;
} ubo;

layout(location = 0) out vec4 outColor;

vec3 unprojectPoint(vec2 ndc, float ndcZ)
{
    vec4 worldPositionH = ubo.inverseViewProjection * vec4(ndc, ndcZ, 1.0);
    // Perspective divide. abs(w) keeps the denominator positive while the small
    // floor protects against numerical issues near degenerate projections.
    return worldPositionH.xyz / max(abs(worldPositionH.w), 1e-6);
}

// Computes anti-aliased coverage for a 2D infinite grid and returns the
// strongest contribution from either line family.
//
// worldCoord is the world position on the plane projected to x/z.
// spacing is the cell size in world units.
// lineWidthPixels is the desired apparent line width in pixels.
//
// fwidth(gridCoord) tells us how much the grid coordinate changes over one
// screen pixel. That derivative is the core anti-aliasing signal: larger values
// mean the grid is shrinking on screen and the transition has to be widened.
float gridLineAlpha(vec2 worldCoord, float spacing, float lineWidthPixels)
{
    vec2 gridCoord = worldCoord / spacing;
    vec2 gridDeriv = max(fwidth(gridCoord), vec2(1e-4));
    // Distance to the nearest vertical and horizontal grid lines in cell space.
    vec2 lineDist = abs(fract(gridCoord - 0.5) - 0.5);
    vec2 lineAlpha = 1.0 - smoothstep(vec2(0.0), gridDeriv * lineWidthPixels, lineDist);
    return max(lineAlpha.x, lineAlpha.y);
}

// Same grid coverage calculation as gridLineAlpha(), but returns separate
// per-axis contributions instead of collapsing them to a single value. This is
// used for the minor grid fade, where each line family must be faded according
// to its own projected pixel density.
vec2 gridLineAlphaPerAxis(vec2 worldCoord, float spacing, float lineWidthPixels)
{
    vec2 gridCoord = worldCoord / spacing;
    vec2 gridDeriv = max(fwidth(gridCoord), vec2(1e-4));
    vec2 lineDist = abs(fract(gridCoord - 0.5) - 0.5);
    return 1.0 - smoothstep(vec2(0.0), gridDeriv * lineWidthPixels, lineDist);
}

void main()
{
    // Convert the raster-space fragment coordinate into normalized device
    // coordinates so it can be unprojected through the inverse view-projection.
    vec2 ndc = vec2(
        (gl_FragCoord.x / max(ubo.viewportSize.x, 1.0)) * 2.0 - 1.0,
        (gl_FragCoord.y / max(ubo.viewportSize.y, 1.0)) * 2.0 - 1.0);

    // Two points on the same screen ray: one on the near clip plane, one on
    // the far clip plane. Their difference is the world-space ray direction.
    vec3 nearPoint = unprojectPoint(ndc, -1.0);
    vec3 farPoint = unprojectPoint(ndc, 1.0);
    vec3 rayDirection = farPoint - nearPoint;

    // Reconstruct the view ray and intersect it against the grid plane at y = 0.
    float denom = rayDirection.y;
    bool hasPlaneHit = abs(denom) > 1e-6;
    float t = hasPlaneHit ? (-nearPoint.y / denom) : 0.0;
    vec3 planeHit = nearPoint + rayDirection * t;

    // Pixels whose rays do not hit the ground plane in front of the camera do
    // not contribute to the grid at all.
    if (!hasPlaneHit || t < 0.0)
    {
        discard;
    }

    // Clamp user-controlled values to keep the shader numerically stable.
    float majorSpacing = max(ubo.spacingAndWidth.x, 1e-4);
    float minorSpacing = max(ubo.spacingAndWidth.y, 1e-4);
    float majorLineWidth = max(ubo.spacingAndWidth.z, 0.25);
    float minorLineWidth = max(ubo.spacingAndWidth.w, 0.25);
    float minorOpacity = clamp(ubo.minorGridParams.x, 0.0, 1.0);
    float minorFadeStartPixels = max(ubo.minorGridParams.y, 0.0);
    float minorFadeEndPixels = max(ubo.minorGridParams.z, minorFadeStartPixels + 1e-4);

    // World-space derivative of the ground-plane hit position over one screen
    // pixel along each grid axis. Larger values mean fewer visible pixels per
    // grid cell and therefore more aggressive fading is needed.
    vec2 planeDeriv = max(fwidth(planeHit.xz), vec2(1e-5));

    // Approximate projected minor-cell size in pixels for each line family.
    // A large value means the minor cell is still clearly visible on screen.
    vec2 minorPixelsPerAxis = minorSpacing / planeDeriv;

    // Major lines always remain visible and rely only on analytic anti-aliasing.
    float majorGrid = gridLineAlpha(planeHit.xz, majorSpacing, majorLineWidth);

    // Minor lines need both anti-aliased line coverage and an additional fade
    // based on their apparent pixel size.
    vec2 minorGridPerAxis = gridLineAlphaPerAxis(planeHit.xz, minorSpacing, minorLineWidth);
    // Fade each line family independently so a heavily foreshortened axis does not
    // suppress both minor-grid directions when the camera is tilted.
    vec2 minorFadePerAxis = smoothstep(vec2(minorFadeStartPixels), vec2(minorFadeEndPixels), minorPixelsPerAxis);
    float minorGrid = max(minorGridPerAxis.x * minorFadePerAxis.x,
                          minorGridPerAxis.y * minorFadePerAxis.y) * minorOpacity;

    // Highlight the world axes on top of the regular grid. These use the same
    // derivative-driven edge widening so the colored axes stay stable at distance.
    float axisX = 1.0 - smoothstep(0.0, max(fwidth(planeHit.x) * 1.5, 1e-4), abs(planeHit.x));
    float axisZ = 1.0 - smoothstep(0.0, max(fwidth(planeHit.z) * 1.5, 1e-4), abs(planeHit.z));

    // Layer the grid from broadest contribution to strongest highlight.
    vec3 baseColor = vec3(0.12, 0.18, 0.24);
    vec3 minorColor = vec3(0.25, 0.33, 0.40);
    vec3 majorColor = vec3(0.34, 0.42, 0.50);
    vec3 axisXColor = vec3(0.85, 0.25, 0.20);
    vec3 axisZColor = vec3(0.20, 0.55, 0.85);

    vec3 color = mix(baseColor, minorColor, minorGrid);
    color = mix(color, majorColor, majorGrid);
    color = mix(color, axisXColor, axisX);
    color = mix(color, axisZColor, axisZ);

    outColor = vec4(color, 1.0);
}
