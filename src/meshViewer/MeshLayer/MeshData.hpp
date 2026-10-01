#pragma once

#include <Geometry/vertex.hpp>

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/pixelTypes.hpp>

#include <QString>
#include <QVector>
#include <QVector4D>
#include <algorithm>
#include <memory>

/** @brief 8-bit RGBA texture image, laid out as 4 contiguous bytes per pixel (matches QRhiTexture::RGBA8). */
using MeshTextureImage = aliceVision::image::Image<aliceVision::image::RGBAColor>;

/**
 * @brief Surface material of a sub-mesh: diffuse color and optional diffuse texture.
 */
struct MeshMaterial
{
    QVector4D baseColor{1.0f, 1.0f, 1.0f, 1.0f}; /**< Diffuse / base color (RGBA), multiplied with the texture when present. */
    int textureIndex = -1;                       /**< Index into MeshData::textures, or -1 when the material is untextured. */
};

/**
 * @brief A mesh chunk sharing a single material.
 */
struct SubMesh
{
    QVector<Vertex> vertices; /**< Positions and normals. */
    QVector<quint32> indices; /**< Triangle indices into vertices (3 per face). */
    QString name;             /**< Sub-mesh name as found in the source file. */
    int materialIndex = -1;   /**< Index into MeshData::materials, or -1 when no material is assigned. */

    /**
     * @brief Texture coordinates, parallel to vertices.
     * @note Left empty unless the source mesh has UVs and its material references a valid texture,
     *       so untextured meshes do not pay the memory cost.
     */
    QVector<UVVertex> uvs;
};

/**
 * @brief CPU-side result of a mesh load: geometry, materials, textures and bounds.
 */
struct MeshData
{
    QVector<SubMesh> subMeshes;
    QVector<MeshMaterial> materials; /**< Materials referenced by SubMesh::materialIndex. */

    /**
     * @brief Decoded texture images referenced by MeshMaterial::textureIndex.
     * @note Held by shared pointer so renderables can keep them alive until GPU upload without copying pixels.
     */
    QVector<std::shared_ptr<const MeshTextureImage>> textures;
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