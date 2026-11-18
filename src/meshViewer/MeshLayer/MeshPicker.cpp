#include <MeshLayer/MeshPicker.hpp>

#include <QDebug>
#include <limits>

namespace {

/**
 * @brief Embree intersect filter rejecting back-facing triangles.
 *
 * Matches the display, which culls back faces with a counter-clockwise front face:
 * a triangle (v0, v1, v2) faces the ray when cross(v1 - v0, v2 - v0) points against the ray direction.
 * The winding is computed from the sub-mesh data rather than from Embree's Ng, so it does not depend
 * on Embree's normal convention.
 * @param[in,out] args Filter arguments; the geometry user data is the SubMesh being intersected.
 */
void cullBackFaces(const RTCFilterFunctionNArguments* args)
{
    const SubMesh* sub = static_cast<const SubMesh*>(args->geometryUserPtr);

    for (unsigned int i = 0; i < args->N; ++i)
    {
        if (args->valid[i] != -1)
        {
            continue;
        }

        const unsigned int primID = RTCHitN_primID(args->hit, args->N, i);
        const Vertex& a = sub->vertices[sub->indices[3 * primID]];
        const Vertex& b = sub->vertices[sub->indices[3 * primID + 1]];
        const Vertex& c = sub->vertices[sub->indices[3 * primID + 2]];

        const QVector3D v0(a.x, a.y, a.z);
        const QVector3D normal = QVector3D::crossProduct(QVector3D(b.x, b.y, b.z) - v0, QVector3D(c.x, c.y, c.z) - v0);
        const QVector3D dir(RTCRayN_dir_x(args->ray, args->N, i), RTCRayN_dir_y(args->ray, args->N, i), RTCRayN_dir_z(args->ray, args->N, i));

        if (QVector3D::dotProduct(normal, dir) >= 0.0f)
        {
            args->valid[i] = 0;
        }
    }
}

}  // namespace

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

        // Ignore back faces, as the display culls them.
        rtcSetGeometryUserData(geom, const_cast<SubMesh*>(&sub));
        rtcSetGeometryIntersectFilterFunction(geom, cullBackFaces);

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
