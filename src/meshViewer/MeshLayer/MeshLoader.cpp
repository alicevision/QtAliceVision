#include <MeshLayer/MeshLoader.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <aliceVision/image/io.hpp>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <algorithm>
#include <exception>
#include <limits>
#include <memory>

namespace {

/**
 * @brief Load a texture file referenced by a material.
 * @param reference Texture path as stored in the material, absolute or relative to the mesh directory.
 * @param meshDir Directory of the mesh file, used to resolve relative texture paths.
 * @return The RGBA image, or nullptr when the texture cannot be loaded.
 * @note Read with aliceVision::image::readImage, converted to sRGB and 8 bits.
 *       Textures embedded in the mesh file (e.g. .glb) are not supported and fail here.
 */
std::shared_ptr<MeshTextureImage> loadTexture(const QString& reference, const QDir& meshDir)
{
    const QString filePath = QDir::cleanPath(meshDir.absoluteFilePath(reference));

    try
    {
        auto image = std::make_shared<MeshTextureImage>();
        aliceVision::image::ImageReadOptions opts;
        opts.workingColorSpace = aliceVision::image::EImageColorSpace::SRGB;
        aliceVision::image::readImage(filePath.toStdString(), *image, opts);
        return image;
    }
    catch (const std::exception& e)
    {
        qWarning() << "MeshLoader: failed to load texture" << filePath << "-" << e.what();
    }

    return nullptr;
}

/**
 * @brief Convert every scene material into a MeshMaterial and load the textures they reference.
 * @param scene The imported scene.
 * @param meshPath Path of the mesh file, used to resolve relative texture paths.
 * @param data Receives the materials and textures.
 *
 * The base color is read from AI_MATKEY_BASE_COLOR (PBR formats) with AI_MATKEY_COLOR_DIFFUSE as fallback,
 * and its alpha from AI_MATKEY_OPACITY. The texture is the first base color or diffuse map.
 * When a diffuse map is used, the color is forced to white: OBJ exporters often write an arbitrary Kd
 * alongside map_Kd, whereas a PBR base color factor is meant to be multiplied with its texture.
 * Textures shared between materials are loaded only once. A texture that fails to load leaves
 * the material untextured instead of failing the whole mesh.
 */
void loadMaterials(const aiScene& scene, const QString& meshPath, MeshData& data)
{
    const QDir meshDir = QFileInfo(meshPath).absoluteDir();
    QHash<QString, int> textureIndexByReference;

    data.materials.reserve(scene.mNumMaterials);

    for (unsigned int m = 0; m < scene.mNumMaterials; ++m)
    {
        const aiMaterial* aiMat = scene.mMaterials[m];
        MeshMaterial material;

        aiColor4D color;
        if (aiMat->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS || aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
        {
            material.baseColor = QVector4D(color.r, color.g, color.b, color.a);
        }

        float opacity = 1.0f;
        if (aiMat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
        {
            material.baseColor.setW(opacity);
        }

        aiString texturePath;
        bool isDiffuseMap = false;
        if (aiMat->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) != AI_SUCCESS)
        {
            isDiffuseMap = (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS);
        }

        const QString reference = QString::fromUtf8(texturePath.C_Str());
        if (!reference.isEmpty())
        {
            auto it = textureIndexByReference.constFind(reference);
            if (it == textureIndexByReference.constEnd())
            {
                int index = -1;
                if (auto image = loadTexture(reference, meshDir))
                {
                    index = static_cast<int>(data.textures.size());
                    data.textures.append(std::move(image));
                }
                it = textureIndexByReference.insert(reference, index);
            }

            material.textureIndex = it.value();

            if (material.textureIndex >= 0 && isDiffuseMap)
            {
                material.baseColor = QVector4D(1.0f, 1.0f, 1.0f, material.baseColor.w());
            }
        }

        data.materials.append(std::move(material));
    }
}

}  // namespace

std::unique_ptr<MeshData> MeshLoader::load(const QString& path)
{
    MeshData result;

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path.toStdString(),
                                             aiProcess_Triangulate |              // triangles only
                                               aiProcess_GenSmoothNormals |       // generate normals if missing
                                               aiProcess_JoinIdenticalVertices |  // deduplicate vertices
                                               aiProcess_FlipUVs |                // Qt / Vulkan UV convention
                                               aiProcess_PreTransformVertices |   // bake node transforms
                                               aiProcess_ValidateDataStructure);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        result.errorString = QString::fromStdString(importer.GetErrorString());
        qWarning() << "MeshLoader: failed to load" << path << "-" << result.errorString;
        return std::make_unique<MeshData>(std::move(result));
    }

    if (scene->mNumMeshes == 0)
    {
        result.errorString = QStringLiteral("No meshes found in file");
        qWarning() << "MeshLoader: no meshes in" << path;
        return std::make_unique<MeshData>(std::move(result));
    }

    loadMaterials(*scene, path, result);

    result.subMeshes.reserve(scene->mNumMeshes);

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
    {
        const aiMesh* mesh = scene->mMeshes[m];
        if (!mesh || mesh->mNumVertices == 0 || mesh->mNumFaces == 0)
            continue;

        SubMesh sub;
        sub.name = QString::fromUtf8(mesh->mName.C_Str());
        sub.vertices.reserve(mesh->mNumVertices);
        sub.indices.reserve(mesh->mNumFaces * 3);

        if (mesh->mMaterialIndex < static_cast<unsigned int>(result.materials.size()))
        {
            sub.materialIndex = static_cast<int>(mesh->mMaterialIndex);
        }

        // UVs are only kept when they will actually be sampled, to save memory on untextured meshes.
        if (mesh->HasTextureCoords(0) && sub.materialIndex >= 0 && result.materials[sub.materialIndex].textureIndex >= 0)
        {
            sub.uvs.reserve(mesh->mNumVertices);
            for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
            {
                // aiProcess_FlipUVs: v = 0 is the first image row, matching the texture upload order.
                sub.uvs.append({mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y});
            }
        }

        // Vertices
        for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
        {
            Vertex vert;

            vert.x = mesh->mVertices[v].x;
            vert.y = mesh->mVertices[v].y;
            vert.z = mesh->mVertices[v].z;

            if (mesh->HasNormals())
            {
                vert.nx = mesh->mNormals[v].x;
                vert.ny = mesh->mNormals[v].y;
                vert.nz = mesh->mNormals[v].z;
            }
            else
            {
                vert.nx = 0.0f;
                vert.ny = 1.0f;
                vert.nz = 0.0f;
            }

            sub.vertices.append(vert);
        }

        // Indices
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace& face = mesh->mFaces[f];
            // aiProcess_Triangulate guarantees 3 indices per face
            sub.indices.append(face.mIndices[0]);
            sub.indices.append(face.mIndices[1]);
            sub.indices.append(face.mIndices[2]);
        }

        result.subMeshes.append(std::move(sub));
    }

    if (result.subMeshes.isEmpty())
    {
        result.errorString = QStringLiteral("No renderable mesh data found");
        qWarning() << "MeshLoader: no renderable data in" << path;
        return std::make_unique<MeshData>(std::move(result));
    }

    computeBounds(result);
    result.valid = true;

    qDebug() << "MeshLoader: loaded" << path << "—" << result.subMeshes.size() << "sub-meshes," << result.materials.size() << "materials,"
             << result.textures.size() << "textures";

    return std::make_unique<MeshData>(std::move(result));
}

void MeshLoader::computeBounds(MeshData& data)
{
    BoundingBox box;
    for (const SubMesh& sub : data.subMeshes)
    {
        for (const Vertex& v : sub.vertices)
        {
            box.extend(QVector3D(v.x, v.y, v.z));
        }
    }

    data.boundingBox = box;
}
