#pragma once

#include <Core/Picking.hpp>
#include <embree4/rtcore.h>
#include <QVector3D>
#include <vector>

struct SfmDataPointInstance;
struct SfmDataCameraInstance;

/** @brief Result of a ray cast against SfM data. */
struct SfmDataPickHit
{
    /** @brief Kind of element hit. */
    enum class Kind
    {
        None,   /**< Nothing hit. */
        Point,  /**< A landmark. */
        Camera, /**< A camera shape. */
    };

    bool hit = false;       /**< True when an element was hit. */
    float distance = 0.0f;  /**< Distance from the ray origin to the hit point. */
    QVector3D worldPoint;   /**< Hit point, in world coordinates. */
    Kind kind = Kind::None; /**< Kind of element hit. */
    size_t index = 0;       /**< Landmark index for points, view id for cameras. */
};

/** @brief Ray caster over landmarks (spheres) and camera shapes (triangles), backed by an Embree BVH. */
class SfmDataPicker
{
  public:
    /** @brief Vertex of the camera picking mesh, laid out for Embree. */
    struct CameraVertex
    {
        float x = 0.0f; /**< X coordinate. */
        float y = 0.0f; /**< Y coordinate. */
        float z = 0.0f; /**< Z coordinate. */
    };

    /** @brief Build a picker with no scene. */
    SfmDataPicker() = default;
    /** @brief Release the Embree scene and device. */
    ~SfmDataPicker();

    /**
     * @brief Build the BVH, replacing any previous one.
     * @param[in] points Landmarks.
     * @param[in] pointRadius Landmark sphere radius.
     * @param[in] cameras Cameras to include.
     * @param[in] cameraScale Scale of the camera shapes, matching the rendered ones.
     */
    void build(const std::vector<SfmDataPointInstance>& points,
               float pointRadius,
               const std::vector<SfmDataCameraInstance>& cameras,
               float cameraScale);
    /** @brief Returns true when a BVH is available. */
    bool isReady() const
    {
        return _scene != nullptr;
    }

    /**
     * @brief Find the closest landmark or camera hit by a ray.
     * @param[in] ray World-space ray.
     * @return The closest hit, or a result with @c hit set to false.
     */
    SfmDataPickHit pick(const Ray& ray) const;

  private:
    /** @brief Landmark sphere, laid out for Embree point geometry. */
    struct SpherePoint
    {
        float x = 0.0f;      /**< Center X coordinate. */
        float y = 0.0f;      /**< Center Y coordinate. */
        float z = 0.0f;      /**< Center Z coordinate. */
        float radius = 0.0f; /**< Sphere radius. */
    };

    /** @brief Release the Embree scene, device and geometry data. */
    void releaseScene();
    /**
     * @brief Create the Embree device and scene.
     * @return False on failure.
     */
    bool createDeviceAndScene();
    /**
     * @brief Add the landmarks as an Embree sphere geometry.
     * @param[in] points Landmarks.
     * @param[in] pointRadius Sphere radius.
     */
    void appendPointGeometry(const std::vector<SfmDataPointInstance>& points, float pointRadius);
    /**
     * @brief Add the camera shapes as an Embree triangle geometry.
     * @param[in] cameras Cameras.
     * @param[in] cameraScale Shape scale.
     */
    void appendCameraGeometry(const std::vector<SfmDataCameraInstance>& cameras, float cameraScale);

    /** @brief Landmark spheres shared with Embree. */
    std::vector<SpherePoint> _spherePoints;
    /** @brief Camera mesh vertices shared with Embree. */
    std::vector<CameraVertex> _cameraVertices;
    /** @brief Camera mesh triangle indices shared with Embree. */
    std::vector<quint32> _cameraIndices;
    /** @brief View id owning each camera triangle. */
    std::vector<size_t> _cameraTriangleOwners;

    /** @brief Embree geometry id of the landmarks. */
    unsigned int _pointGeomID = RTC_INVALID_GEOMETRY_ID;
    /** @brief Embree geometry id of the cameras. */
    unsigned int _cameraGeomID = RTC_INVALID_GEOMETRY_ID;

    /** @brief Embree device. */
    RTCDevice _device = nullptr;
    /** @brief Embree scene. */
    RTCScene _scene = nullptr;
};