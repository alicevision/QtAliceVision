#pragma once

#include <Core/Picking.hpp>
#include <embree4/rtcore.h>
#include <QVector3D>
#include <vector>

struct SfmDataPointInstance;
struct SfmDataCameraInstance;

struct SfmDataPickHit
{
    enum class Kind
    {
        None,
        Point,
        Camera,
    };

    bool hit = false;
    float distance = 0.0f;
    QVector3D worldPoint;
    Kind kind = Kind::None;
    size_t index = 0;
};

class SfmDataPicker
{
  public:
    struct CameraVertex
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    SfmDataPicker() = default;
    ~SfmDataPicker();

    void build(const std::vector<SfmDataPointInstance>& points,
               float pointRadius,
               const std::vector<SfmDataCameraInstance>& cameras,
               float cameraScale);
    bool isReady() const
    {
        return _scene != nullptr;
    }

    SfmDataPickHit pick(const Ray& ray) const;
    void setSelectedObject(const SfmDataPickHit& hit);
    void clearSelectedObject();
    const SfmDataPickHit& selectedObject() const
    {
        return _selectedObject;
    }

  private:
    struct SpherePoint
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float radius = 0.0f;
    };

    void releaseScene();
    bool createDeviceAndScene();
    void appendPointGeometry(const std::vector<SfmDataPointInstance>& points, float pointRadius);
    void appendCameraGeometry(const std::vector<SfmDataCameraInstance>& cameras, float cameraScale);

    std::vector<SpherePoint> _spherePoints;
    std::vector<CameraVertex> _cameraVertices;
    std::vector<quint32> _cameraIndices;
    std::vector<size_t> _cameraTriangleOwners;

    unsigned int _pointGeomID = RTC_INVALID_GEOMETRY_ID;
    unsigned int _cameraGeomID = RTC_INVALID_GEOMETRY_ID;

    SfmDataPickHit _selectedObject;

    RTCDevice _device = nullptr;
    RTCScene _scene = nullptr;
};