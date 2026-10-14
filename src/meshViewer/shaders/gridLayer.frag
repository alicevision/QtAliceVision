#version 460

/**
 * @file gridLayer.frag
 * @brief Full-screen procedural ground grid with adaptive levels of detail.
 *
 * The vertex stage renders a screen-filling quad. This fragment stage then:
 * 1. Reconstructs the world-space view ray for the current pixel and
 *    intersects it against the ground plane y = 0.
 * 2. Picks grid levels whose spacing is baseSpacing * subdivisions^k, based on
 *    how many pixels a cell covers on screen. Three consecutive levels are
 *    drawn; the finest fades out and the middle one turns from minor to major
 *    as the camera zooms out, so level changes never pop.
 * 3. Draws each level with an energy-conserving line function ("pristine
 *    grid", Ben Golus): lines never get thinner than one pixel, sub-pixel
 *    lines lose opacity instead, and lines denser than one per pixel converge
 *    to their average coverage instead of producing moire.
 * 4. Fades the grid out at grazing angles near the horizon.
 */

layout(std140, binding = 0) uniform GridUniformBuffer {
    /** Inverse of projection * view, used to unproject screen positions. */
    mat4 inverseViewProjection;
    /** xy = viewport size in pixels. zw = unused padding. */
    vec4 viewportSize;
    /**
     * x = base (finest) spacing in world units, y = subdivisions between
     * consecutive levels, z = major line width in pixels, w = minor line width
     * in pixels.
     */
    vec4 spacingAndWidth;
    /** x = minor opacity, y = minimum cell size in pixels. zw = unused padding. */
    vec4 minorGridParams;
} ubo;

layout(location = 0) out vec4 outColor;

/**
 * @brief Unprojects a normalized device coordinate back to world space.
 * @param ndc Screen position in normalized device coordinates.
 * @param ndcZ Depth in normalized device coordinates. 0 and 1 are inside the
 *        clip volume for both the [-1, 1] and [0, 1] depth conventions.
 * @return The world-space position.
 */
vec3 unprojectPoint(vec2 ndc, float ndcZ)
{
    vec4 worldPositionH = ubo.inverseViewProjection * vec4(ndc, ndcZ, 1.0);
    return worldPositionH.xyz / worldPositionH.w;
}

/**
 * @brief Anti-aliased coverage of a line centered on gridUV = 0, per axis.
 * @param gridUV Twice the distance to the line center, in line units.
 * @param uvDeriv Size of one pixel in line units.
 * @param targetWidth Requested full line width in line units.
 * @param maxWidth Upper bound for the drawn width in line units.
 * @return Coverage in [0, 1] for each axis.
 * @note The drawn width is clamped to at least one pixel; thinner requested
 *       lines are compensated by lowering the opacity, which keeps the
 *       perceived brightness constant and avoids flickering.
 */
vec2 lineCoverage(vec2 gridUV, vec2 uvDeriv, vec2 targetWidth, float maxWidth)
{
    vec2 drawWidth = clamp(targetWidth, uvDeriv, vec2(maxWidth));
    vec2 lineAA = max(uvDeriv, vec2(1e-6)) * 1.5;
    vec2 coverage = 1.0 - smoothstep(drawWidth - lineAA, drawWidth + lineAA, gridUV);
    return coverage * clamp(targetWidth / drawWidth, 0.0, 1.0);
}

/**
 * @brief Coverage of one grid level with lines of constant pixel width.
 * @param worldCoord Position on the ground plane (world x/z).
 * @param worldDeriv Size of one pixel in world units along x and z.
 * @param spacing Cell size of this level in world units.
 * @param lineWidthPixels Desired line width in pixels.
 * @return Combined coverage of both line families in [0, 1].
 */
float gridLevelCoverage(vec2 worldCoord, vec2 worldDeriv, float spacing, float lineWidthPixels)
{
    vec2 uv = worldCoord / spacing;
    vec2 uvDeriv = worldDeriv / spacing;
    vec2 targetWidth = min(uvDeriv * lineWidthPixels, vec2(0.5));
    vec2 gridUV = 1.0 - abs(fract(uv) * 2.0 - 1.0);
    vec2 coverage = lineCoverage(gridUV, uvDeriv, targetWidth, 0.5);
    // Once a cell gets close to one pixel, converge to the average coverage of
    // the cell instead of sampling individual lines.
    coverage = mix(coverage, targetWidth, clamp(uvDeriv * 2.0 - 1.0, 0.0, 1.0));
    return coverage.x + coverage.y - coverage.x * coverage.y;
}

void main()
{
    vec2 ndc = vec2(
        (gl_FragCoord.x / max(ubo.viewportSize.x, 1.0)) * 2.0 - 1.0,
        (gl_FragCoord.y / max(ubo.viewportSize.y, 1.0)) * 2.0 - 1.0);

    vec3 nearPoint = unprojectPoint(ndc, 0.0);
    vec3 farPoint = unprojectPoint(ndc, 1.0);
    vec3 rayDirection = farPoint - nearPoint;

    float denom = rayDirection.y;
    bool hasPlaneHit = abs(denom) > 1e-9;
    float t = hasPlaneHit ? (-nearPoint.y / denom) : 0.0;
    vec3 planeHit = nearPoint + rayDirection * t;

    // Derivatives are evaluated before any discard so that they stay defined
    // for the whole pixel quad.
    vec2 worldDeriv = vec2(length(vec2(dFdx(planeHit.x), dFdy(planeHit.x))),
                           length(vec2(dFdx(planeHit.z), dFdy(planeHit.z))));
    worldDeriv = max(worldDeriv, vec2(1e-9));

    if (!hasPlaneHit || t < 0.0)
    {
        discard;
    }

    float baseSpacing = max(ubo.spacingAndWidth.x, 1e-6);
    float subdivisions = max(ubo.spacingAndWidth.y, 2.0);
    float majorLineWidth = max(ubo.spacingAndWidth.z, 0.25);
    float minorLineWidth = max(ubo.spacingAndWidth.w, 0.25);
    float minorOpacity = clamp(ubo.minorGridParams.x, 0.0, 1.0);
    float minCellPixels = max(ubo.minorGridParams.y, 1.0);

    // Continuous level of detail: level k (spacing baseSpacing * subdivisions^k)
    // is fully visible once its cells cover minCellPixels * subdivisions pixels
    // and invisible below minCellPixels. The cell size is the geometric mean of
    // both axes so that, with a tilted camera, the foreshortened axis does not
    // switch the far grid to coarse levels too early; that axis is still kept
    // alias-free by the per-axis filtering in gridLevelCoverage().
    // The level is clamped so that baseSpacing stays the finest grid drawn.
    float worldPerPixel = sqrt(worldDeriv.x * worldDeriv.y);
    float lod = max(log(minCellPixels * worldPerPixel / baseSpacing) / log(subdivisions), -1.0);
    float lodFloor = floor(lod);

    vec3 baseColor = vec3(0.12, 0.18, 0.24);
    vec3 minorColor = vec3(0.25, 0.33, 0.40);
    vec3 majorColor = vec3(0.34, 0.42, 0.50);
    vec3 axisXColor = vec3(0.85, 0.25, 0.20);
    vec3 axisZColor = vec3(0.20, 0.55, 0.85);

    // Fade the grid in the last degree before the horizon, where even filtered
    // lines turn to noise.
    float grazing = abs(rayDirection.y) / max(length(rayDirection), 1e-9);
    float horizonFade = smoothstep(0.0, 0.01, grazing);

    // Composite from the finest to the coarsest level. Each coarser level is a
    // subset of the finer ones and is drawn on top, which keeps the result
    // continuous when lodFloor changes.
    vec3 color = baseColor;
    for (int i = 1; i <= 3; ++i)
    {
        float level = lodFloor + float(i);
        float visibility = clamp(level - lod, 0.0, 1.0);
        float emphasis = clamp(level - lod - 1.0, 0.0, 1.0);
        float spacing = baseSpacing * pow(subdivisions, level);
        float lineWidth = mix(minorLineWidth, majorLineWidth, emphasis);
        float coverage = gridLevelCoverage(planeHit.xz, worldDeriv, spacing, lineWidth);
        float alpha = coverage * visibility * mix(minorOpacity, 1.0, emphasis) * horizonFade;
        color = mix(color, mix(minorColor, majorColor, emphasis), alpha);
    }

    // World axes: the X axis is the line z = 0 and the Z axis is the line x = 0.
    vec2 axisWidth = worldDeriv * majorLineWidth * 1.5;
    vec2 axisCoverage = lineCoverage(2.0 * abs(planeHit.zx), worldDeriv.yx, axisWidth.yx, 3.0e38) * horizonFade;
    color = mix(color, axisXColor, axisCoverage.x);
    color = mix(color, axisZColor, axisCoverage.y);

    outColor = vec4(color, 1.0);
}
