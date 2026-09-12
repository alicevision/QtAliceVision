#pragma once

#include <algorithm>
#include <QVector>
#include <cmath>
#include "vertex.hpp"

/**
 * @brief Builds a UV sphere mesh into @p verts and @p indices.
 *
 * The sphere is centred at the origin. Instance offsets (world-space centres)
 * are applied externally in the vertex shader via a per-instance buffer.
 *
 * @param verts   Output vertex array (cleared on entry).
 * @param indices Output index array (cleared on entry).
 * @param radius  Sphere radius.
 * @param r,g,b   Uniform RGB colour applied to all vertices.
 * @param stacks  Number of horizontal rings (latitude subdivisions).
 * @param slices  Number of vertical columns (longitude subdivisions).
 */
inline void buildSphereMesh(
    QVector<ColoredVertex> &verts,
    QVector<quint32> &indices,
    float radius = 0.05f,
    float r = 1.0f, float g = 1.0f, float b = 0.0f,
    int stacks = 12, int slices = 18)
{
    verts.clear();
    indices.clear();

    auto addVert = [&](float x, float y, float z) -> quint32 {
        quint32 idx = static_cast<quint32>(verts.size());
        verts.append({x, y, z, r, g, b});
        return idx;
    };

    // Top pole
    quint32 topPole = addVert(0.f, radius, 0.f);

    // Intermediate rings (phi from just below top to just above bottom)
    for (int i = 1; i < stacks; ++i) {
        const float phi = float(M_PI * i / stacks);
        const float y   = radius * std::cos(phi);
        const float rr  = radius * std::sin(phi);
        for (int j = 0; j < slices; ++j) {
            const float theta = float(2.0 * M_PI * j / slices);
            addVert(rr * std::cos(theta), y, rr * std::sin(theta));
        }
    }

    // Bottom pole
    quint32 botPole = addVert(0.f, -radius, 0.f);

    // Top cap: fan around top pole
    for (int j = 0; j < slices; ++j) {
        const quint32 cur  = 1 + j;
        const quint32 next = 1 + (j + 1) % slices;
        indices.append(topPole);
        indices.append(cur);
        indices.append(next);
    }

    // Middle quads (stacks-2 bands between adjacent rings)
    for (int i = 0; i < stacks - 2; ++i) {
        const quint32 base = 1 + static_cast<quint32>(i * slices);
        for (int j = 0; j < slices; ++j) {
            const quint32 next = static_cast<quint32>((j + 1) % slices);
            const quint32 a = base + j;                     // ring i,   col j
            const quint32 b = base + next;                  // ring i,   col j+1
            const quint32 c = base + slices + j;            // ring i+1, col j
            const quint32 d = base + slices + next;         // ring i+1, col j+1
            indices.append(a); indices.append(b); indices.append(c);
            indices.append(b); indices.append(d); indices.append(c);
        }
    }

    // Bottom cap: fan around bottom pole
    const quint32 lastRingBase = 1 + static_cast<quint32>((stacks - 2) * slices);
    for (int j = 0; j < slices; ++j) {
        const quint32 cur  = lastRingBase + j;
        const quint32 next = lastRingBase + (j + 1) % slices;
        indices.append(botPole);
        indices.append(next);
        indices.append(cur);
    }
}

inline void buildSphereMesh(
    QVector<PositionVertex> &verts,
    QVector<quint32> &indices,
    float radius,
    int stacks = 12, int slices = 18)
{
    verts.clear();
    indices.clear();

    auto addVert = [&](float x, float y, float z) -> quint32 {
        const quint32 idx = static_cast<quint32>(verts.size());
        verts.append({x, y, z});
        return idx;
    };

    quint32 topPole = addVert(0.f, radius, 0.f);

    for (int i = 1; i < stacks; ++i) {
        const float phi = float(M_PI * i / stacks);
        const float y = radius * std::cos(phi);
        const float rr = radius * std::sin(phi);
        for (int j = 0; j < slices; ++j) {
            const float theta = float(2.0 * M_PI * j / slices);
            addVert(rr * std::cos(theta), y, rr * std::sin(theta));
        }
    }

    quint32 botPole = addVert(0.f, -radius, 0.f);

    for (int j = 0; j < slices; ++j) {
        const quint32 cur = 1 + j;
        const quint32 next = 1 + (j + 1) % slices;
        indices.append(topPole);
        indices.append(cur);
        indices.append(next);
    }

    for (int i = 0; i < stacks - 2; ++i) {
        const quint32 base = 1 + static_cast<quint32>(i * slices);
        for (int j = 0; j < slices; ++j) {
            const quint32 next = static_cast<quint32>((j + 1) % slices);
            const quint32 a = base + j;
            const quint32 b = base + next;
            const quint32 c = base + slices + j;
            const quint32 d = base + slices + next;
            indices.append(a); indices.append(b); indices.append(c);
            indices.append(b); indices.append(d); indices.append(c);
        }
    }

    const quint32 lastRingBase = 1 + static_cast<quint32>((stacks - 2) * slices);
    for (int j = 0; j < slices; ++j) {
        const quint32 cur = lastRingBase + j;
        const quint32 next = lastRingBase + (j + 1) % slices;
        indices.append(botPole);
        indices.append(next);
        indices.append(cur);
    }
}

/**
 * @brief Builds a simple camera-frustum mesh into @p verts and @p indices.
 *
 * The camera origin sits at the frustum apex and looks along +Z. The field of
 * view is interpreted as a vertical FOV in degrees. The image plane is placed
 * at @p depth, with width derived from @p aspect.
 *
 * @param verts    Output vertex array (cleared on entry).
 * @param indices  Output index array (cleared on entry).
 * @param fovDeg   Vertical field of view in degrees.
 * @param depth    Distance from the apex to the image plane.
 * @param aspect   Image-plane aspect ratio (width / height).
 * @param r,g,b    Uniform RGB colour applied to all vertices.
 */
inline void buildCameraMesh(
    QVector<ColoredVertex> &verts,
    QVector<quint32> &indices,
    float fovDeg,
    float depth = 0.02f,
    float aspect = 4.0f / 3.0f,
    float r = 1.0f, float g = 0.8f, float b = 0.0f)
{
    verts.clear();
    indices.clear();

    const float clampedFov = std::clamp(fovDeg, 1.0f, 179.0f);
    const float clampedAspect = std::max(aspect, 0.01f);
    const float clampedDepth = std::max(depth, 0.0001f);
    const float halfFovRad = clampedFov * float(M_PI / 360.0);
    const float halfHeight = clampedDepth * std::tan(halfFovRad);
    const float halfWidth = halfHeight * clampedAspect;

    auto addVert = [&](float x, float y, float z) -> quint32 {
        const quint32 idx = static_cast<quint32>(verts.size());
        verts.append({x, y, z, r, g, b});
        return idx;
    };

    const quint32 apex = addVert(0.f, 0.f, 0.f);
    const quint32 topLeft = addVert(-halfWidth, halfHeight, clampedDepth);
    const quint32 topRight = addVert(halfWidth, halfHeight, clampedDepth);
    const quint32 bottomRight = addVert(halfWidth, -halfHeight, clampedDepth);
    const quint32 bottomLeft = addVert(-halfWidth, -halfHeight, clampedDepth);

    // Side faces.
    indices.append(apex); indices.append(topLeft); indices.append(topRight);
    indices.append(apex); indices.append(topRight); indices.append(bottomRight);
    indices.append(apex); indices.append(bottomRight); indices.append(bottomLeft);
    indices.append(apex); indices.append(bottomLeft); indices.append(topLeft);

    // Image-plane cap.
    indices.append(topLeft); indices.append(bottomLeft); indices.append(bottomRight);
    indices.append(topLeft); indices.append(bottomRight); indices.append(topRight);
}

inline void buildSphereWireframeMesh(
    QVector<ColoredVertex> &verts,
    QVector<quint32> &indices,
    float radius = 0.05f,
    float r = 1.0f, float g = 1.0f, float b = 0.0f,
    int stacks = 12, int slices = 18)
{
    verts.clear();
    indices.clear();

    auto addVert = [&](float x, float y, float z) -> quint32 {
        const quint32 idx = static_cast<quint32>(verts.size());
        verts.append({x, y, z, r, g, b});
        return idx;
    };

    const quint32 topPole = addVert(0.f, radius, 0.f);

    for (int i = 1; i < stacks; ++i) {
        const float phi = float(M_PI * i / stacks);
        const float y = radius * std::cos(phi);
        const float rr = radius * std::sin(phi);
        for (int j = 0; j < slices; ++j) {
            const float theta = float(2.0 * M_PI * j / slices);
            addVert(rr * std::cos(theta), y, rr * std::sin(theta));
        }
    }

    const quint32 botPole = addVert(0.f, -radius, 0.f);

    for (int ring = 0; ring < stacks - 1; ++ring) {
        const quint32 base = 1 + static_cast<quint32>(ring * slices);
        for (int slice = 0; slice < slices; ++slice) {
            const quint32 current = base + static_cast<quint32>(slice);
            const quint32 next = base + static_cast<quint32>((slice + 1) % slices);
            indices.append(current);
            indices.append(next);
        }
    }

    for (int slice = 0; slice < slices; ++slice) {
        const quint32 firstRing = 1 + static_cast<quint32>(slice);
        indices.append(topPole);
        indices.append(firstRing);
    }

    for (int ring = 0; ring < stacks - 2; ++ring) {
        const quint32 base = 1 + static_cast<quint32>(ring * slices);
        const quint32 nextBase = base + static_cast<quint32>(slices);
        for (int slice = 0; slice < slices; ++slice) {
            indices.append(base + static_cast<quint32>(slice));
            indices.append(nextBase + static_cast<quint32>(slice));
        }
    }

    const quint32 lastRingBase = 1 + static_cast<quint32>((stacks - 2) * slices);
    for (int slice = 0; slice < slices; ++slice) {
        indices.append(lastRingBase + static_cast<quint32>(slice));
        indices.append(botPole);
    }
}

inline void buildCameraWireframeMesh(
    QVector<ColoredVertex> &verts,
    QVector<quint32> &indices,
    float fovDeg,
    float depth = 0.12f,
    float aspect = 4.0f / 3.0f)
{
    verts.clear();
    indices.clear();

    const float clampedFov = std::clamp(fovDeg, 1.0f, 179.0f);
    const float clampedAspect = std::max(aspect, 0.01f);
    const float clampedDepth = std::max(depth, 0.0001f);
    const float halfFovRad = clampedFov * float(M_PI / 360.0);
    const float halfHeight = clampedDepth * std::tan(halfFovRad);
    const float halfWidth = halfHeight * clampedAspect;
    const float axisWidth = halfWidth * 2.0f;

    auto addVert = [&](float x, float y, float z, float r = 1.0f, float g = 1.0f, float b = 1.0f) -> quint32 {
        const quint32 idx = static_cast<quint32>(verts.size());
        verts.append({x, -y, -z, r, g, b});
        return idx;
    };

    const quint32 apex = addVert(0.f, 0.f, 0.f);
    const quint32 topLeft = addVert(-halfWidth, halfHeight, clampedDepth);
    const quint32 topRight = addVert(halfWidth, halfHeight, clampedDepth);
    const quint32 bottomRight = addVert(halfWidth, -halfHeight, clampedDepth);
    const quint32 bottomLeft = addVert(-halfWidth, -halfHeight, clampedDepth);

    const quint32 x1 = addVert(0.0f, 0.0f, 0.0f, 1.0, 0.0f, 0.0f);
    const quint32 x2 = addVert(axisWidth, 0.0f, 0.0f, 1.0, 0.0f, 0.0f);
    const quint32 y1 = addVert(0.0f, 0.0f, 0.0f, 0.0, 1.0f, 0.0f);
    const quint32 y2 = addVert(0.0f, axisWidth, 0.0f, 0.0, 1.0f, 0.0f);
    const quint32 z1 = addVert(0.0f, 0.0f, 0.0f, 0.0, 0.0f, 1.0f);
    const quint32 z2 = addVert(0.0f, 0.0f, axisWidth, 0.0, 0.0f, 1.0f);

    indices.append(apex); indices.append(topLeft);
    indices.append(apex); indices.append(topRight);
    indices.append(apex); indices.append(bottomRight);
    indices.append(apex); indices.append(bottomLeft);

    indices.append(topLeft); indices.append(topRight);
    indices.append(topRight); indices.append(bottomRight);
    indices.append(bottomRight); indices.append(bottomLeft);
    indices.append(bottomLeft); indices.append(topLeft);

    indices.append(x1); indices.append(x2);
    indices.append(y1); indices.append(y2);
    indices.append(z1); indices.append(z2);
}
