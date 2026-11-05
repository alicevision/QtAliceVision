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
#include <cstddef>
#include <exception>
#include <limits>
#include <memory>
#include <string>

namespace {

/**
 * @brief Build an RGBA image from raw 8-bit BGRA pixels.
 * @param bgra Pixels, 4 bytes per pixel in B, G, R, A order, row-major from the first row.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @return The RGBA image.
 */
std::shared_ptr<MeshTextureImage> imageFromBGRA8(const unsigned char* bgra, int width, int height)
{
    auto image = std::make_shared<MeshTextureImage>(width, height);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const unsigned char* texel = bgra + (static_cast<std::size_t>(y) * width + x) * 4;
            (*image)(y, x) = aliceVision::image::RGBAColor(texel[2], texel[1], texel[0], texel[3]);
        }
    }
    return image;
}

/**
 * @brief Decode a texture embedded in the mesh file (e.g. glTF binary .glb).
 * @param texture The embedded texture.
 * @param opts Read options passed to AliceVision for compressed textures.
 * @return The RGBA image.
 * @throws std::exception when the texture cannot be decoded.
 *
 * A compressed texture (mHeight == 0) holds an encoded image file of mWidth bytes, decoded by
 * aliceVision::image::readImageFromMemory. Its format comes from achFormatHint, where Assimp keeps at most
 * 3 characters: "web" stands for WebP (glTF EXT_texture_webp). An uncompressed texture holds raw BGRA8 texels.
 */
std::shared_ptr<MeshTextureImage> decodeEmbeddedTexture(const aiTexture& texture, const aliceVision::image::ImageReadOptions& opts)
{
    if (texture.mHeight > 0)
    {
        return imageFromBGRA8(reinterpret_cast<const unsigned char*>(texture.pcData), static_cast<int>(texture.mWidth), static_cast<int>(texture.mHeight));
    }

    std::string formatHint(texture.achFormatHint);
    if (formatHint == "web")
    {
        formatHint = "webp";
    }

    auto image = std::make_shared<MeshTextureImage>();
    aliceVision::image::readImageFromMemory(texture.pcData, texture.mWidth, formatHint, *image, opts);
    return image;
}

/**
 * @brief Check that a decoded texture can be uploaded.
 * @param image Decoded image, possibly null.
 * @param reference Texture reference, for the warning.
 * @return The image, or nullptr when it is null or empty (a 0x0 GPU texture cannot be created).
 */
std::shared_ptr<MeshTextureImage> rejectEmptyImage(std::shared_ptr<MeshTextureImage> image, const QString& reference)
{
    if (image && (image->width() <= 0 || image->height() <= 0))
    {
        qWarning() << "MeshLoader: empty texture" << reference;
        return nullptr;
    }
    return image;
}

/**
 * @brief Load a texture referenced by a material.
 * @param scene The imported scene, used to resolve textures embedded in the mesh file.
 * @param reference Texture path as stored in the material: an embedded texture reference (e.g. "*0" in a .glb),
 *                  or a file path, absolute or relative to the mesh directory.
 * @param meshDir Directory of the mesh file, used to resolve relative texture paths.
 * @return The RGBA image, or nullptr when the texture cannot be loaded.
 * @note Decoded with AliceVision (readImage for files, readImageFromMemory for embedded textures), converted to
 *       sRGB and 8 bits. readImageFromMemory requires an AliceVision build including it (branch
 *       dev/readImageFromMemory, not yet in develop).
 * @note An empty (0x0) image is rejected like a decoding failure.
 */
std::shared_ptr<MeshTextureImage> loadTexture(const aiScene& scene, const QString& reference, const QDir& meshDir)
{
    aliceVision::image::ImageReadOptions opts;
    opts.workingColorSpace = aliceVision::image::EImageColorSpace::SRGB;

    const QByteArray referenceUtf8 = reference.toUtf8();
    if (const aiTexture* embedded = scene.GetEmbeddedTexture(referenceUtf8.constData()))
    {
        try
        {
            return rejectEmptyImage(decodeEmbeddedTexture(*embedded, opts), reference);
        }
        catch (const std::exception& e)
        {
            qWarning() << "MeshLoader: failed to decode embedded texture" << reference << "-" << e.what();
        }

        return nullptr;
    }

    const QString filePath = QDir::cleanPath(meshDir.absoluteFilePath(reference));

    try
    {
        auto image = std::make_shared<MeshTextureImage>();
        aliceVision::image::readImage(filePath.toStdString(), *image, opts);
        return rejectEmptyImage(std::move(image), filePath);
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
                if (auto image = loadTexture(scene, reference, meshDir))
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
                                               aiProcess_SortByPType |            // split points / lines from triangles
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
        {
            continue;
        }

        // Points and lines (e.g. left by a degenerate triangle) are not rendered.
        if ((mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0)
        {
            continue;
        }

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
            // aiProcess_SortByPType keeps triangles apart, but a mesh may still mix primitive types.
            if (face.mNumIndices != 3)
            {
                continue;
            }
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
