#pragma once

#include <MeshLayer/MeshData.hpp>
#include <Core/Picking.hpp>

#include <embree4/rtcore.h>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>

struct HitResult
{
    bool hit = false;
    float distance = 0.0f;
    unsigned int geomID = 0;  // sub-mesh index
    unsigned int primID = 0;  // triangle index within that sub-mesh
    QVector3D normal;         // geometric normal, world space
};

class MeshPicker
{
  public:
    MeshPicker() = default;
    ~MeshPicker();

    void buildBVH(const MeshData& meshData);

    bool isReady() const
    {
        return _scene != nullptr;
    }

    HitResult pick(const Ray& ray) const;

  private:
    void releaseScene();

    const MeshData* _meshData = nullptr;

    RTCDevice _device = nullptr;
    RTCScene _scene = nullptr;
};
