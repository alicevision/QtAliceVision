#pragma once

#include <MeshLayer/MeshData.hpp>

#include <QString>
#include <QVector>
#include <algorithm>
#include <memory>

class MeshLoader
{
  public:
    static std::unique_ptr<MeshData> load(const QString& path);

  private:
    static void computeBounds(MeshData& data);
};