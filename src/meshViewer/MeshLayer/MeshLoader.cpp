#include <MeshLayer/MeshLoader.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <QDebug>
#include <algorithm>
#include <limits>
#include <memory>

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

    qDebug() << "MeshLoader: loaded" << path << "—" << result.subMeshes.size() << "sub-meshes";

    return std::make_unique<MeshData>(std::move(result));
}

void MeshLoader::computeBounds(MeshData& data)
{
    if (data.subMeshes.isEmpty())
    {
        data.minX = data.maxX = 0.0f;
        data.minY = data.maxY = 0.0f;
        data.minZ = data.maxZ = 0.0f;
        return;
    }

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    float maxZ = -std::numeric_limits<float>::max();

    for (const SubMesh& sub : data.subMeshes)
    {
        for (const Vertex& v : sub.vertices)
        {
            minX = std::min(minX, v.x);
            maxX = std::max(maxX, v.x);
            minY = std::min(minY, v.y);
            maxY = std::max(maxY, v.y);
            minZ = std::min(minZ, v.z);
            maxZ = std::max(maxZ, v.z);
        }
    }

    data.minX = minX;
    data.maxX = maxX;
    data.minY = minY;
    data.maxY = maxY;
    data.minZ = minZ;
    data.maxZ = maxZ;
}