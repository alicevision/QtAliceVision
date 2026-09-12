#include <SfmDataLayer/SfmDataPicker.hpp>

#include <Geometry/Geometry.hpp>
#include <SfmDataLayer/SfmDataObject.hpp>

#include <QDebug>
#include <QMatrix4x4>

#include <aliceVision/system/Logger.hpp>

#include <limits>

namespace {

void appendTriangleMesh(std::vector<SfmDataPicker::CameraVertex>& dstVerts,
                        std::vector<quint32>& dstIndices,
                        const QVector<ColoredVertex>& srcVerts,
                        const QVector<quint32>& srcIndices,
                        const QMatrix4x4& transform)
{
    const quint32 baseIndex = static_cast<quint32>(dstVerts.size());

    for (const ColoredVertex& vert : srcVerts)
    {
        const QVector3D position = transform.map(QVector3D(vert.x, vert.y, vert.z));
        dstVerts.push_back({position.x(), position.y(), position.z()});
    }

    for (quint32 index : srcIndices)
    {
        dstIndices.push_back(baseIndex + index);
    }
}

void alignCameraMeshToRenderedBasis(QVector<ColoredVertex>& verts)
{
    for (ColoredVertex& vert : verts)
    {
        vert.y = -vert.y;
        vert.z = -vert.z;
    }
}

}  // namespace

SfmDataPicker::~SfmDataPicker()
{
    releaseScene();
}

void SfmDataPicker::releaseScene()
{
    if (_scene)
    {
        rtcReleaseScene(_scene);
        _scene = nullptr;
    }

    if (_device)
    {
        rtcReleaseDevice(_device);
        _device = nullptr;
    }

    _spherePoints.clear();
    _cameraVertices.clear();
    _cameraIndices.clear();
    _cameraTriangleOwners.clear();
    _pointGeomID = RTC_INVALID_GEOMETRY_ID;
    _cameraGeomID = RTC_INVALID_GEOMETRY_ID;
    _selectedObject = {};
}

bool SfmDataPicker::createDeviceAndScene()
{
    _device = rtcNewDevice(nullptr);
    if (!_device)
    {
        qWarning() << "SfmDataPicker: failed to create Embree device";
        return false;
    }

    _scene = rtcNewScene(_device);
    if (!_scene)
    {
        qWarning() << "SfmDataPicker: failed to create Embree scene";
        rtcReleaseDevice(_device);
        _device = nullptr;
        return false;
    }

    return true;
}

void SfmDataPicker::build(const std::vector<SfmDataPointInstance>& points,
                          float pointRadius,
                          const std::vector<SfmDataCameraInstance>& cameras,
                          float cameraScale)
{
    releaseScene();

    if (points.empty() && cameras.empty())
    {
        return;
    }

    if (!createDeviceAndScene())
    {
        return;
    }

    // appendPointGeometry(points, pointRadius);
    appendCameraGeometry(cameras, cameraScale);

    rtcCommitScene(_scene);

    qDebug() << "SfmDataPicker: BVH built for" << _spherePoints.size() << "point primitive(s) and" << (_cameraIndices.size() / 3)
             << "camera triangle(s)";
}

void SfmDataPicker::appendPointGeometry(const std::vector<SfmDataPointInstance>& points, float pointRadius)
{
    if (points.empty())
    {
        return;
    }

    _spherePoints.reserve(points.size());
    const float clampedRadius = std::max(pointRadius, 0.001f);
    for (const SfmDataPointInstance& point : points)
    {
        _spherePoints.push_back({point.position.x(), point.position.y(), point.position.z(), clampedRadius});
    }

    RTCGeometry geom = rtcNewGeometry(_device, RTC_GEOMETRY_TYPE_SPHERE_POINT);
    rtcSetSharedGeometryBuffer(
      geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4, _spherePoints.data(), 0, sizeof(SpherePoint), _spherePoints.size());

    rtcCommitGeometry(geom);
    _pointGeomID = rtcAttachGeometry(_scene, geom);
    rtcReleaseGeometry(geom);
}

void SfmDataPicker::appendCameraGeometry(const std::vector<SfmDataCameraInstance>& cameras, float cameraScale)
{
    if (cameras.empty())
    {
        return;
    }

    for (size_t cameraIndex = 0; cameraIndex < cameras.size(); ++cameraIndex)
    {
        const SfmDataCameraInstance& camera = cameras[cameraIndex];
        const float scaledSphereRadius = std::max(sphereRadius * cameraScale, 0.001f);
        const float scaledDepth = std::max(cameraDepth * cameraScale, 0.0001f);
        const float fovDegrees = camera.intrinsics->getHorizontalFov() * 180.0 / M_PI;
        const float aspectRatio = static_cast<float>(camera.intrinsics->w()) / static_cast<float>(camera.intrinsics->h());

        QMatrix4x4 transform = toQMatrix4x4(camera.camera_T_world.inverse());

        if (fovDegrees > 180.0f)
        {
            QVector<PositionVertex> sphereVerts;
            QVector<quint32> sphereIndices;
            buildSphereMesh(sphereVerts, sphereIndices, scaledSphereRadius);

            const quint32 baseIndex = static_cast<quint32>(_cameraVertices.size());
            const QVector3D translation = transform.column(3).toVector3D();
            for (const PositionVertex& vert : sphereVerts)
            {
                _cameraVertices.push_back({vert.x + translation.x(), vert.y + translation.y(), vert.z + translation.z()});
            }
            for (quint32 index : sphereIndices)
            {
                _cameraIndices.push_back(baseIndex + index);
            }
            _cameraTriangleOwners.insert(_cameraTriangleOwners.end(), sphereIndices.size() / 3, cameraIndex);
            continue;
        }

        QVector<ColoredVertex> localVerts;
        QVector<quint32> localIndices;
        buildCameraMesh(localVerts, localIndices, fovDegrees, scaledDepth, aspectRatio);

        alignCameraMeshToRenderedBasis(localVerts);
        appendTriangleMesh(_cameraVertices, _cameraIndices, localVerts, localIndices, transform);
        _cameraTriangleOwners.insert(_cameraTriangleOwners.end(), localIndices.size() / 3, cameraIndex);
    }

    if (_cameraVertices.empty() || _cameraIndices.empty())
    {
        return;
    }

    RTCGeometry geom = rtcNewGeometry(_device, RTC_GEOMETRY_TYPE_TRIANGLE);
    rtcSetSharedGeometryBuffer(
      geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, _cameraVertices.data(), 0, sizeof(CameraVertex), _cameraVertices.size());

    rtcSetSharedGeometryBuffer(
      geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, _cameraIndices.data(), 0, 3 * sizeof(quint32), _cameraIndices.size() / 3);

    rtcCommitGeometry(geom);
    _cameraGeomID = rtcAttachGeometry(_scene, geom);
    rtcReleaseGeometry(geom);
}

SfmDataPickHit SfmDataPicker::pick(const Ray& ray) const
{
    SfmDataPickHit result;

    if (!_scene)
    {
        return result;
    }

    const QVector3D dir = ray.direction.normalized();

    RTCRayHit rayhit{};
    rayhit.ray.org_x = ray.origin.x();
    rayhit.ray.org_y = ray.origin.y();
    rayhit.ray.org_z = ray.origin.z();
    rayhit.ray.dir_x = dir.x();
    rayhit.ray.dir_y = dir.y();
    rayhit.ray.dir_z = dir.z();
    rayhit.ray.tnear = 0.0f;
    rayhit.ray.tfar = std::numeric_limits<float>::infinity();
    rayhit.ray.mask = 0xFFFFFFFFu;
    rayhit.ray.flags = 0;
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rayhit.hit.primID = RTC_INVALID_GEOMETRY_ID;
    rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    RTCIntersectArguments args;
    rtcInitIntersectArguments(&args);
    rtcIntersect1(_scene, &rayhit, &args);

    if (rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID || rayhit.hit.primID == RTC_INVALID_GEOMETRY_ID)
    {
        return result;
    }

    result.hit = true;
    result.distance = rayhit.ray.tfar;
    result.worldPoint = ray.origin + dir * result.distance;

    if (rayhit.hit.geomID == _pointGeomID)
    {
        ALICEVISION_LOG_INFO("point");
        const size_t pointIndex = static_cast<size_t>(rayhit.hit.primID);
        if (pointIndex >= _spherePoints.size())
        {
            return {};
        }

        result.kind = SfmDataPickHit::Kind::Point;
        result.index = pointIndex;
        return result;
    }

    if (rayhit.hit.geomID == _cameraGeomID)
    {
        ALICEVISION_LOG_INFO("camera");
        const size_t triangleIndex = static_cast<size_t>(rayhit.hit.primID);
        if (triangleIndex >= _cameraTriangleOwners.size())
        {
            return {};
        }

        result.kind = SfmDataPickHit::Kind::Camera;
        result.index = _cameraTriangleOwners[triangleIndex];
        return result;
    }

    return {};
}

void SfmDataPicker::setSelectedObject(const SfmDataPickHit& hit)
{
    _selectedObject = hit;
}

void SfmDataPicker::clearSelectedObject()
{
    _selectedObject = {};
}