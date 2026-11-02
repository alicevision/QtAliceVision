#pragma once

#include <MeshLayer/MeshData.hpp>

#include <QString>
#include <QVector>
#include <algorithm>
#include <memory>

/** @brief Loads a mesh file (any format supported by Assimp) into a MeshData. */
class MeshLoader
{
  public:
    /**
     * @brief Load a mesh with its materials and textures.
     * @param[in] path Local path of the mesh file.
     * @return The mesh data; check MeshData::valid and MeshData::errorString.
     */
    static std::unique_ptr<MeshData> load(const QString& path);

  private:
    /**
     * @brief Compute MeshData::boundingBox from the vertex positions.
     * @param[in,out] data Mesh data to update.
     */
    static void computeBounds(MeshData& data);
};