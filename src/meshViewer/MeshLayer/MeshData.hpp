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
 * @brief Surface material of a sub-mesh: base color, optional base color texture and PBR metallic-roughness parameters.
 *
 * The PBR parameters are only used by the MeshPBR shading mode. Its metallic and roughness are the factors multiplied
 * with the metallic-roughness texture when present (glTF layout: G = roughness, B = metallic), the factors alone otherwise.
 */
struct MeshMaterial
{
    /**
     * @brief Base color factor, linear RGBA, multiplied with the (linearly sampled) base color texture when present.
     * @note Always linear, whatever the source format: see loadMaterials() in MeshLoader.cpp.
     */
    QVector4D baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    int textureIndex = -1;                       /**< Index into MeshData::textures, or -1 when the material is untextured. */

    /**
     * @brief Index into MeshData::textures of the metallic-roughness texture, or -1 when there is none.
     * @note Linear data (decoded without color conversion), in glTF channel layout: G = roughness, B = metallic.
     */
    int metallicRoughnessTextureIndex = -1;

    /** @brief Metallic factor in [0, 1]. Defaults to a dielectric surface when the file defines no PBR material. */
    float metallicFactor = 0.0f;

    /** @brief Roughness factor in [0, 1]. Defaults to a fully rough (diffuse) surface when the file defines none. */
    float roughnessFactor = 1.0f;

    /**
     * @brief True when the base color alpha (factor and texture) must be ignored: glTF alphaMode OPAQUE.
     * @note False when the file defines no alpha mode (e.g. OBJ), so base color alpha keeps blending as before.
     *       The layer opacity applies in both cases.
     */
    bool opaque = false;
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
     * @note Left empty unless the source mesh has UVs and its material references a valid texture
     *       (base color or metallic-roughness), so untextured meshes do not pay the memory cost.
     */
    QVector<UVVertex> uvs;
};

/**
 * @brief CPU-side result of a mesh load: geometry, materials, textures and bounds.
 */
struct MeshData
{
    QVector<SubMesh> subMeshes;      /**< Sub-meshes, one per material in the source file. */
    QVector<MeshMaterial> materials; /**< Materials referenced by SubMesh::materialIndex. */

    /**
     * @brief Decoded texture images referenced by MeshMaterial::textureIndex and MeshMaterial::metallicRoughnessTextureIndex.
     * @note Held by shared pointer so renderables can keep them alive until GPU upload without copying pixels.
     */
    QVector<std::shared_ptr<const MeshTextureImage>> textures;
    QString errorString;             /**< Error message when loading failed. */
    bool valid = false;              /**< True when loading succeeded. */

    /** @brief Bounds of the vertex positions; empty when there is no geometry. */
    BoundingBox boundingBox;
};
