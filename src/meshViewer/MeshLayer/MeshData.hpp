#pragma once

#include <QVector>
#include <Geometry/vertex.hpp>

struct SubMesh
{
    QVector<Vertex> vertices;
    QVector<quint32> indices;
    QString name;
};

struct MeshData
{
    QVector<SubMesh> subMeshes;
    QString errorString;
    bool valid = false;

    // Bounding box
    float minX = 0, maxX = 0;
    float minY = 0, maxY = 0;
    float minZ = 0, maxZ = 0;

    float centerX() const
    {
        return (minX + maxX) * 0.5f;
    }
    float centerY() const
    {
        return (minY + maxY) * 0.5f;
    }
    float centerZ() const
    {
        return (minZ + maxZ) * 0.5f;
    }

    float extentX() const
    {
        return maxX - minX;
    }
    float extentY() const
    {
        return maxY - minY;
    }
    float extentZ() const
    {
        return maxZ - minZ;
    }

    float maxExtent() const
    {
        return std::max({extentX(), extentY(), extentZ()});
    }
};