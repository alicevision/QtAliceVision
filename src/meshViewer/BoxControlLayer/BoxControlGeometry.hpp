#pragma once

#include <QVector>
#include <QVector3D>
#include <cmath>

#include <Geometry/Geometry.hpp>
#include <Geometry/vertex.hpp>

/**
 * Dimensions of the box control gizmo, in gizmo units (the gizmo is scaled to keep a constant screen size).
 * Values reproduce the former Qt3D TransformGizmo.
 */
namespace boxControl {

constexpr float centerRadius = 0.04f;
constexpr float shaftStart = centerRadius;
constexpr float shaftEnd = 0.54f;
constexpr float shaftRadius = 0.011f;
constexpr float scaleCubeEdge = 0.06f;
constexpr float coneLength = 0.13f;
constexpr float coneRadius = 0.035f;
constexpr float torusRadius = 0.75f;
constexpr float torusMinorRadius = 0.011f;
constexpr float faceRadius = 0.04f;
/** Distance between a box face and its face handle, in world units. */
constexpr float faceOffset = 0.3f;
/** Translate handles sit this much farther from the box center than the face handles. */
constexpr float translateHandleRatio = 1.25f;

/**
 * @brief Returns the world distance from the box center to the base of a translate handle.
 * It is proportional to the box size so that the handle stays outside the box, beyond the face handle.
 * @param halfExtent Box half extent along the handle axis.
 */
inline float translateHandleDistance(float halfExtent)
{
    return translateHandleRatio * (halfExtent + faceOffset);
}

/** @brief Returns the X/Y/Z axis color (red/green/blue). */
inline QVector3D axisColor(int axis)
{
    switch (axis)
    {
        case 0:
            return {0.902f, 0.231f, 0.333f};  // #e63b55
        case 1:
            return {0.514f, 0.769f, 0.078f};  // #83c414
        default:
            return {0.200f, 0.529f, 0.886f};  // #3387e2
    }
}

/** @brief Returns the unit vector of @p axis (0 = X, 1 = Y, 2 = Z). */
inline QVector3D axisVector(int axis)
{
    return QVector3D(axis == 0 ? 1.f : 0.f, axis == 1 ? 1.f : 0.f, axis == 2 ? 1.f : 0.f);
}

/** @brief Returns the radius of the rotation torus of @p axis, slightly shrunk for Y/Z to avoid overlapping rings. */
inline float torusRadiusForAxis(int axis)
{
    const float shrink = 2.f * torusMinorRadius + 0.01f;
    return torusRadius * (1.f - static_cast<float>(axis) * shrink);
}

/** @brief Builds two unit vectors orthogonal to @p dir and to each other. */
inline void orthonormalFrame(const QVector3D& dir, QVector3D& u, QVector3D& v)
{
    const QVector3D ref = std::abs(dir.y()) < 0.9f ? QVector3D(0, 1, 0) : QVector3D(1, 0, 0);
    u = QVector3D::crossProduct(dir, ref).normalized();
    v = QVector3D::crossProduct(dir, u).normalized();
}

/**
 * @brief Appends a closed truncated cone (cylinder when both radii are equal) along @p dir.
 * @param start,end Distances along @p dir of the two caps.
 * @param r0,r1 Radii at @p start and @p end.
 */
inline void appendTube(QVector<ColoredVertex>& verts,
                       QVector<quint32>& indices,
                       const QVector3D& dir,
                       float start,
                       float end,
                       float r0,
                       float r1,
                       const QVector3D& color,
                       int sides = 12)
{
    QVector3D u, v;
    orthonormalFrame(dir, u, v);

    auto addVert = [&](const QVector3D& p) -> quint32 {
        const quint32 idx = static_cast<quint32>(verts.size());
        verts.append(ColoredVertex{p.x(), p.y(), p.z(), color.x(), color.y(), color.z()});
        return idx;
    };

    const quint32 c0 = addVert(dir * start);
    const quint32 c1 = addVert(dir * end);
    const quint32 ring0 = static_cast<quint32>(verts.size());
    for (int i = 0; i < sides; ++i)
    {
        const float a = float(2.0 * M_PI * i / sides);
        addVert(dir * start + (u * std::cos(a) + v * std::sin(a)) * r0);
        addVert(dir * end + (u * std::cos(a) + v * std::sin(a)) * r1);
    }

    for (int i = 0; i < sides; ++i)
    {
        const quint32 b0 = ring0 + 2 * i;
        const quint32 t0 = b0 + 1;
        const quint32 b1 = ring0 + 2 * ((i + 1) % sides);
        const quint32 t1 = b1 + 1;
        indices << b0 << b1 << t0 << t0 << b1 << t1;
        indices << c0 << b1 << b0;
        indices << c1 << t0 << t1;
    }
}

/** @brief Appends an axis-aligned cube of edge @p edge centered at @p center. */
inline void appendCube(QVector<ColoredVertex>& verts, QVector<quint32>& indices, const QVector3D& center, float edge, const QVector3D& color)
{
    const float h = 0.5f * edge;
    const quint32 base = static_cast<quint32>(verts.size());
    for (int i = 0; i < 8; ++i)
    {
        const QVector3D p = center + QVector3D((i & 1) ? h : -h, (i & 2) ? h : -h, (i & 4) ? h : -h);
        verts.append(ColoredVertex{p.x(), p.y(), p.z(), color.x(), color.y(), color.z()});
    }

    static const quint32 cubeIndices[36] = {0, 2, 1, 1, 2, 3, 4, 5, 6, 5, 7, 6, 0, 1, 4, 1, 5, 4,
                                            2, 6, 3, 3, 6, 7, 0, 4, 2, 2, 4, 6, 1, 3, 5, 3, 7, 5};
    for (quint32 idx : cubeIndices)
    {
        indices.append(base + idx);
    }
}

/** @brief Appends a torus of radius @p radius around @p axisDir, centered at the origin. */
inline void appendTorus(QVector<ColoredVertex>& verts,
                        QVector<quint32>& indices,
                        const QVector3D& axisDir,
                        float radius,
                        float minorRadius,
                        const QVector3D& color,
                        int rings = 48,
                        int sides = 8)
{
    QVector3D u, v;
    orthonormalFrame(axisDir, u, v);

    const quint32 base = static_cast<quint32>(verts.size());
    for (int i = 0; i < rings; ++i)
    {
        const float a = float(2.0 * M_PI * i / rings);
        const QVector3D radial = u * std::cos(a) + v * std::sin(a);
        for (int j = 0; j < sides; ++j)
        {
            const float b = float(2.0 * M_PI * j / sides);
            const QVector3D p = radial * (radius + minorRadius * std::cos(b)) + axisDir * (minorRadius * std::sin(b));
            verts.append(ColoredVertex{p.x(), p.y(), p.z(), color.x(), color.y(), color.z()});
        }
    }

    for (int i = 0; i < rings; ++i)
    {
        const int ni = (i + 1) % rings;
        for (int j = 0; j < sides; ++j)
        {
            const int nj = (j + 1) % sides;
            const quint32 a = base + i * sides + j;
            const quint32 b = base + ni * sides + j;
            const quint32 c = base + ni * sides + nj;
            const quint32 d = base + i * sides + nj;
            indices << a << b << c << a << c << d;
        }
    }
}

/** @brief Appends a sphere of radius @p radius centered at @p center. */
inline void appendSphere(QVector<ColoredVertex>& verts, QVector<quint32>& indices, const QVector3D& center, float radius, const QVector3D& color)
{
    QVector<ColoredVertex> sphereVerts;
    QVector<quint32> sphereIndices;
    buildSphereMesh(sphereVerts, sphereIndices, radius, color.x(), color.y(), color.z(), 8, 12);

    const quint32 base = static_cast<quint32>(verts.size());
    for (ColoredVertex vtx : sphereVerts)
    {
        vtx.x += center.x();
        vtx.y += center.y();
        vtx.z += center.z();
        verts.append(vtx);
    }
    for (quint32 idx : sphereIndices)
    {
        indices.append(base + idx);
    }
}

/** @brief Builds the 12 triangles of the unit cube [-1, 1]^3, in white. */
inline void buildBoxFaces(QVector<ColoredVertex>& verts, QVector<quint32>& indices)
{
    verts.clear();
    indices.clear();
    // Edge 2 cube centered at the origin
    appendCube(verts, indices, QVector3D(), 2.0f, QVector3D(1.f, 1.f, 1.f));
}

/** @brief Builds the 12 edges of the unit cube [-1, 1]^3 as a line list (24 vertices), in orange. */
inline void buildBoxEdges(QVector<ColoredVertex>& verts)
{
    verts.clear();
    const QVector3D orange(0.957f, 0.608f, 0.169f);  // #f49b2b

    for (int axis = 0; axis < 3; ++axis)
    {
        const int a1 = (axis + 1) % 3;
        const int a2 = (axis + 2) % 3;
        for (int k = 0; k < 4; ++k)
        {
            QVector3D p0, p1;
            p0[axis] = -1.f;
            p1[axis] = 1.f;
            p0[a1] = p1[a1] = (k & 1) ? 1.f : -1.f;
            p0[a2] = p1[a2] = (k & 2) ? 1.f : -1.f;
            verts.append(ColoredVertex{p0.x(), p0.y(), p0.z(), orange.x(), orange.y(), orange.z()});
            verts.append(ColoredVertex{p1.x(), p1.y(), p1.z(), orange.x(), orange.y(), orange.z()});
        }
    }
}

}  // namespace boxControl
