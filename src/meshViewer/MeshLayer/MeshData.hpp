#pragma once

#include <Core/BoundingBox.hpp>
#include <Geometry/vertex.hpp>

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/pixelTypes.hpp>

#include <QString>
#include <QVector>
#include <QVector4D>
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

    /** @brief Bounds of the vertex positions; empty when there is no geometry. */
    BoundingBox boundingBox;
};
