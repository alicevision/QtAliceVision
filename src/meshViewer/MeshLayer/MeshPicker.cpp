#include <MeshLayer/MeshPicker.hpp>

#include <QDebug>
#include <limits>

MeshPicker::~MeshPicker()
{
    releaseScene();
}

void MeshPicker::releaseScene()
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

    _meshData = nullptr;
}

void MeshPicker::buildBVH(const MeshData& meshData)
{
    releaseScene();

    if (!meshData.valid)
    {
        qWarning() << "MeshPicker::buildBVH: invalid mesh data";
        return;
    }

    _meshData = &meshData;

    _device = rtcNewDevice(nullptr);
    if (!_device)
    {
        qWarning() << "MeshPicker::buildBVH: failed to create Embree device";
        return;
    }

    _scene = rtcNewScene(_device);

    for (const SubMesh& sub : _meshData->subMeshes)
    {
        if (sub.vertices.isEmpty() || sub.indices.isEmpty())
        {
            continue;
        }

        RTCGeometry geom = rtcNewGeometry(_device, RTC_GEOMETRY_TYPE_TRIANGLE);

        // Share buffers directly — no copy.
        // Vertex: Embree reads float3 positions; our Vertex struct starts with x,y,z.
        rtcSetSharedGeometryBuffer(geom,
                                   RTC_BUFFER_TYPE_VERTEX,
                                   0,
                                   RTC_FORMAT_FLOAT3,
                                   sub.vertices.constData(),
                                   0,               // byte offset of first element
                                   sizeof(Vertex),  // stride — skips over nx,ny,nz
                                   static_cast<size_t>(sub.vertices.size()));

        // Index: packed uint32 triplets, one per triangle.
        rtcSetSharedGeometryBuffer(geom,
                                   RTC_BUFFER_TYPE_INDEX,
                                   0,
                                   RTC_FORMAT_UINT3,
                                   sub.indices.constData(),
                                   0,
                                   3 * sizeof(quint32),
                                   static_cast<size_t>(sub.indices.size() / 3));

        rtcCommitGeometry(geom);
        rtcAttachGeometry(_scene, geom);
        rtcReleaseGeometry(geom);  // scene holds its own reference
    }

    rtcCommitScene(_scene);  // triggers BVH build

    qDebug() << "MeshPicker: BVH built for" << _meshData->subMeshes.size() << "sub-mesh(es)";
}

HitResult MeshPicker::pick(const Ray& ray) const
{
    HitResult result;

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
    rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    RTCIntersectArguments args;
    rtcInitIntersectArguments(&args);
    rtcIntersect1(_scene, &rayhit, &args);

    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
    {
        result.hit = true;
        result.distance = rayhit.ray.tfar;
        result.geomID = rayhit.hit.geomID;
        result.primID = rayhit.hit.primID;
        result.normal = QVector3D(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z).normalized();
    }

    return result;
}
