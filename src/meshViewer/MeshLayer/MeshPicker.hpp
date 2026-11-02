#pragma once

#include <MeshLayer/MeshData.hpp>
#include <Core/Picking.hpp>

#include <embree4/rtcore.h>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>

/** @brief Result of a ray cast against a mesh. */
struct HitResult
{
    bool hit = false;         /**< True when a triangle was hit. */
    float distance = 0.0f;    /**< Distance from the ray origin to the hit point. */
    unsigned int geomID = 0;  /**< Sub-mesh index. */
    unsigned int primID = 0;  /**< Triangle index within the sub-mesh. */
    QVector3D normal;         /**< Geometric normal, in world space. */
};

/** @brief Ray caster over a MeshData, backed by an Embree BVH. */
class MeshPicker
{
  public:
    /** @brief Build a picker with no scene. */
    MeshPicker() = default;
    /** @brief Release the Embree scene and device. */
    ~MeshPicker();

    /**
     * @brief Build the BVH of every sub-mesh, replacing any previous one.
     * @param[in] meshData Mesh to index. Must outlive the picker or the next buildBVH() call.
     */
    void buildBVH(const MeshData& meshData);

    /** @brief Returns true when a BVH is available. */
    bool isReady() const
    {
        return _scene != nullptr;
    }

    /**
     * @brief Find the closest triangle hit by a ray.
     * @param[in] ray World-space ray.
     * @return The closest hit, or a result with @c hit set to false.
     */
    HitResult pick(const Ray& ray) const;

  private:
    /** @brief Release the Embree scene and device. */
    void releaseScene();

    /** @brief Indexed mesh, not owned. */
    const MeshData* _meshData = nullptr;

    /** @brief Embree device. */
    RTCDevice _device = nullptr;
    /** @brief Embree scene holding one geometry per sub-mesh. */
    RTCScene _scene = nullptr;
};
