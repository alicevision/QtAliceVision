#pragma once

#include <Core/BoundingBox.hpp>
#include <Geometry/vertex.hpp>

#include <QString>
#include <QVector>


struct DepthmapData
{
    QVector<ColoredVertex> vertices;
    QVector<quint32> indices;
    QString errorString;
    bool valid = false;

    /** @brief Bounds of the vertex positions; empty when there is no geometry. */
    BoundingBox boundingBox;
};
