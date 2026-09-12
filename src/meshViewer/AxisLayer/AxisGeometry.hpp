#pragma once

#include <QVector>
#include <QVector3D>
#include <cmath>
#include <Geometry/vertex.hpp>

/**
 * @brief Appends one arrow along @p axisDir to the given vertex/index arrays.
 *
 * The arrow consists of a cylindrical shaft followed by a cone arrowhead.
 * All geometry is centred at the origin; translation is applied externally via MVP.
 *
 * @param verts    Output vertex array (appended to).
 * @param indices  Output index array (appended to).
 * @param axisDir  Unit vector along the arrow axis.
 * @param refUp    Any unit vector not parallel to @p axisDir (used to build the radial frame).
 * @param shaftLen Length of the cylindrical shaft.
 * @param shaftR   Radius of the cylindrical shaft.
 * @param coneLen  Length of the cone arrowhead.
 * @param coneR    Base radius of the cone arrowhead.
 * @param r,g,b    RGB color applied to all vertices of this arrow.
 * @param sides    Number of sides on the cylinder/cone.
 */
static void appendArrow(QVector<ColoredVertex>& verts,
                        QVector<quint32>& indices,
                        QVector3D axisDir,
                        QVector3D refUp,
                        float shaftLen,
                        float shaftR,
                        float coneLen,
                        float coneR,
                        float r,
                        float g,
                        float b,
                        int sides = 10)
{
    // Build orthonormal radial frame
    QVector3D u = QVector3D::crossProduct(axisDir, refUp).normalized();
    QVector3D v = QVector3D::crossProduct(axisDir, u).normalized();

    // Add a vertex and return its index
    auto addVert = [&](QVector3D pos) -> quint32 {
        quint32 idx = static_cast<quint32>(verts.size());
        verts.append(ColoredVertex{pos.x(), pos.y(), pos.z(), r, g, b});
        return idx;
    };

    // Return position on a ring at distance `t` along axis, at `angle`
    auto ringPos = [&](float t, float angle, float radius) { return axisDir * t + u * (radius * std::cos(angle)) + v * (radius * std::sin(angle)); };

    // ---- shaft: cylinder from 0 to shaftLen ----

    // Bottom ring (at origin)
    QVector<quint32> botRing, topRing;
    for (int i = 0; i < sides; ++i)
    {
        float a = float(2.0 * M_PI * i / sides);
        botRing.append(addVert(ringPos(0.f, a, shaftR)));
    }
    // Top ring (at shaftLen)
    for (int i = 0; i < sides; ++i)
    {
        float a = float(2.0 * M_PI * i / sides);
        topRing.append(addVert(ringPos(shaftLen, a, shaftR)));
    }

    // Bottom cap (facing away from tip)
    quint32 botCenter = addVert(QVector3D(0.f, 0.f, 0.f));
    for (int i = 0; i < sides; ++i)
    {
        int j = (i + 1) % sides;
        indices.append(botCenter);
        indices.append(botRing[j]);
        indices.append(botRing[i]);
    }

    // Shaft side quads (two triangles each)
    for (int i = 0; i < sides; ++i)
    {
        int j = (i + 1) % sides;
        indices.append(botRing[i]);
        indices.append(botRing[j]);
        indices.append(topRing[i]);

        indices.append(topRing[i]);
        indices.append(botRing[j]);
        indices.append(topRing[j]);
    }

    // ---- arrowhead: cone from shaftLen to shaftLen + coneLen ----

    // Cone base ring (wider than shaft)
    QVector<quint32> coneBase;
    for (int i = 0; i < sides; ++i)
    {
        float a = float(2.0 * M_PI * i / sides);
        coneBase.append(addVert(ringPos(shaftLen, a, coneR)));
    }

    // Cone base cap (facing back toward origin)
    quint32 coneBaseCtr = addVert(axisDir * shaftLen);
    for (int i = 0; i < sides; ++i)
    {
        int j = (i + 1) % sides;
        indices.append(coneBaseCtr);
        indices.append(coneBase[j]);
        indices.append(coneBase[i]);
    }

    // Cone lateral surface
    quint32 tip = addVert(axisDir * (shaftLen + coneLen));
    for (int i = 0; i < sides; ++i)
    {
        int j = (i + 1) % sides;
        indices.append(coneBase[i]);
        indices.append(coneBase[j]);
        indices.append(tip);
    }
}

/**
 * @brief Builds a complete XYZ axis gizmo mesh into @p verts and @p indices.
 *
 * - X axis → red   (1.0, 0.15, 0.15)
 * - Y axis → green (0.15, 1.0,  0.15)
 * - Z axis → blue  (0.15, 0.4,  1.0 )
 *
 * All three arrows share the origin. The @p scale parameter multiplies all
 * shaft/cone dimensions uniformly. Translate the result via MVP to place the
 * gizmo at an arbitrary world position.
 *
 * @param verts   Output vertex array (cleared on entry).
 * @param indices Output index array (cleared on entry).
 * @param scale   Uniform scale factor applied to shaft and cone dimensions.
 */
inline void buildAxisMesh(QVector<ColoredVertex>& verts, QVector<quint32>& indices, float scale = 1.0f)
{
    verts.clear();
    indices.clear();

    const float shaftLen = 0.75f * scale;
    const float shaftR = 0.03f * scale;
    const float coneLen = 0.25f * scale;
    const float coneR = 0.08f * scale;

    // X axis — red
    appendArrow(verts, indices, {1, 0, 0}, {0, 1, 0}, shaftLen, shaftR, coneLen, coneR, 0.7f, 0.15f, 0.15f);

    // Y axis — green
    appendArrow(verts, indices, {0, 1, 0}, {1, 0, 0}, shaftLen, shaftR, coneLen, coneR, 0.15f, 0.7f, 0.15f);

    // Z axis — blue
    appendArrow(verts, indices, {0, 0, 1}, {0, 1, 0}, shaftLen, shaftR, coneLen, coneR, 0.15f, 0.4f, 0.7f);
}
