#pragma once

#include <Core/BoundingBox.hpp>
#include <Geometry/vertex.hpp>

#include <QString>
#include <QVector>

/** @brief Triangle mesh built from a depth map, ready for upload. */
struct DepthmapData
{
    QVector<ColoredVertex> vertices; /**< World-space vertices, jet-colored by similarity (or by normalized depth without sim map). */
    QVector<quint32> indices;        /**< Triangle indices. */
    QString errorString;             /**< Error message when loading failed. */
    bool valid = false;              /**< True when loading succeeded. */

    /** @brief Bounds of the vertex positions; empty when there is no geometry. */
    BoundingBox boundingBox;
};
