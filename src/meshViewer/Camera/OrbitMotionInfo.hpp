#pragma once

#include <aliceVision/geometry/Pose3.hpp>
#include <Core/BoundingBox.hpp>
#include <Core/MotionInfo.hpp>

/**
 * @brief Camera motion orbiting around a center point, with pan and dolly.
 *
 * The camera state is split in two parts:
 *  - a committed state: rotation (_pose), orbit center (_center) and distance (_distance);
 *  - pending interaction deltas exposed as properties (relativeRotationX/Y, planeX/Y, distance),
 *    applied on top of the committed state while the user drags.
 *
 * applyTransform() folds the pending deltas into the committed state and resets them to zero.
 */
class OrbitMotionInfo : public MotionInfo
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Pending rotation around the camera X axis, in degrees. */
    Q_PROPERTY(float relativeRotationX READ relativeRotationX WRITE setRelativeRotationX NOTIFY relativeRotationXChanged)
    /** @brief Pending rotation around the camera Y axis, in degrees. */
    Q_PROPERTY(float relativeRotationY READ relativeRotationY WRITE setRelativeRotationY NOTIFY relativeRotationYChanged)
    /** @brief Pending horizontal pan, as a fraction of the orbit distance. */
    Q_PROPERTY(float planeX READ planeX WRITE setPlaneX NOTIFY planeXChanged)
    /** @brief Pending vertical pan, as a fraction of the orbit distance. */
    Q_PROPERTY(float planeY READ planeY WRITE setPlaneY NOTIFY planeYChanged)
    /** @brief Pending change of the orbit distance, in world units. */
    Q_PROPERTY(float distance READ distance WRITE setDeltaDistance NOTIFY distanceChanged)

  public:
    /**
     * @brief Build a motion at unit distance from the origin, looking along -Z.
     * @param[in] parent Optional Qt parent object.
     */
    explicit OrbitMotionInfo(QObject* parent = nullptr);

    /** @brief Commit the pending rotation, pan and distance deltas and reset them to zero. */
    Q_INVOKABLE void applyTransform();
    /**
     * @brief Move the orbit center while keeping the camera position.
     *
     * The camera is re-oriented to look at the new center and the distance is updated accordingly.
     *
     * @param[in] center New orbit center, in world coordinates.
     */
    Q_INVOKABLE void setCenter(const QVector3D& center);
    /**
     * @brief Set the committed orbit distance.
     * @param[in] distance Distance to the center, in world units.
     */
    Q_INVOKABLE void setDistance(float distance);
    /**
     * @brief Frame a bounding box from a default three-quarter view.
     * @param[in] bounds Box to frame. Ignored when empty.
     */
    Q_INVOKABLE void fit(const BoundingBox& bounds);

    /** @brief Orient the camera to look down the -Y axis (top view). */
    Q_INVOKABLE void viewTop();
    /** @brief Orient the camera to look up the +Y axis (bottom view). */
    Q_INVOKABLE void viewBottom();
    /** @brief Orient the camera to look along -Z (front view). */
    Q_INVOKABLE void viewFront();
    /** @brief Orient the camera to look along +Z (back view). */
    Q_INVOKABLE void viewBack();
    /** @brief Orient the camera to look along +X (left view). */
    Q_INVOKABLE void viewLeft();
    /** @brief Orient the camera to look along -X (right view). */
    Q_INVOKABLE void viewRight();

    /** @brief Get the pending rotation around the camera X axis, in degrees. */
    float relativeRotationX() const
    {
        return _relativeRotationX;
    }

    /** @brief Get the pending rotation around the camera Y axis, in degrees. */
    float relativeRotationY() const
    {
        return _relativeRotationY;
    }

    /** @brief Get the pending horizontal pan, as a fraction of the orbit distance. */
    float planeX() const
    {
        return _planeX;
    }

    /** @brief Get the pending vertical pan, as a fraction of the orbit distance. */
    float planeY() const
    {
        return _planeY;
    }

    /** @brief Get the pending change of the orbit distance. */
    float distance() const
    {
        return _deltaDistance;
    }

    /**
     * @brief Set the pending rotation around the camera X axis.
     * @param[in] v Angle, in degrees.
     */
    void setRelativeRotationX(float v)
    {
        if (qFuzzyCompare(_relativeRotationX, v))
        {
            return;
        }
        _relativeRotationX = v;
        emit relativeRotationXChanged();
        emit changed();
    }

    /**
     * @brief Set the pending rotation around the camera Y axis.
     * @param[in] v Angle, in degrees.
     */
    void setRelativeRotationY(float v)
    {
        if (qFuzzyCompare(_relativeRotationY, v))
        {
            return;
        }
        _relativeRotationY = v;
        emit relativeRotationYChanged();
        emit changed();
    }

    /**
     * @brief Set the pending horizontal pan.
     * @param[in] v Pan, as a fraction of the orbit distance.
     */
    void setPlaneX(float v)
    {
        if (qFuzzyCompare(_planeX, v))
        {
            return;
        }
        _planeX = v;
        emit planeXChanged();
        emit changed();
    }

    /**
     * @brief Set the pending vertical pan.
     * @param[in] v Pan, as a fraction of the orbit distance.
     */
    void setPlaneY(float v)
    {
        if (qFuzzyCompare(_planeY, v))
        {
            return;
        }
        _planeY = v;
        emit planeYChanged();
        emit changed();
    }

    /**
     * @brief Set the pending change of the orbit distance.
     * @param[in] v Distance delta, in world units.
     */
    void setDeltaDistance(float v)
    {
        if (qFuzzyCompare(_deltaDistance, v))
        {
            return;
        }
        _deltaDistance = v;
        emit distanceChanged();
        emit changed();
    }

    /**
     * @brief Compare two motions by their view matrix.
     * @param[in] other Motion to compare with.
     * @return True if both matrices are equal.
     */
    bool operator==(const OrbitMotionInfo& other) const
    {
        return getMatrix() == other.getMatrix();
    }

    /** @brief Returns the view matrix, including pending deltas. */
    QMatrix4x4 getMatrix() const override;
    /** @brief Returns the view (world to camera) matrix, including pending deltas. */
    Eigen::Matrix4d getEigenMatrix() const;
    /** @brief Returns the orbit center, including the pending pan, in world coordinates. */
    Eigen::Vector3d getCenter() const override;

  signals:
    /** @brief Emitted when the pending X rotation changes. */
    void relativeRotationXChanged();
    /** @brief Emitted when the pending Y rotation changes. */
    void relativeRotationYChanged();
    /** @brief Emitted when the pending horizontal pan changes. */
    void planeXChanged();
    /** @brief Emitted when the pending vertical pan changes. */
    void planeYChanged();
    /** @brief Emitted when the pending or committed distance changes. */
    void distanceChanged();

  private:
    /** @brief Pending rotation around the camera X axis, in degrees. */
    float _relativeRotationX = 0.0f;
    /** @brief Pending rotation around the camera Y axis, in degrees. */
    float _relativeRotationY = 0.0f;
    /** @brief Pending horizontal pan, as a fraction of the orbit distance. */
    float _planeX = 0.0f;
    /** @brief Pending vertical pan, as a fraction of the orbit distance. */
    float _planeY = 0.0f;
    /** @brief Pending change of the orbit distance. */
    float _deltaDistance = 0.0f;

    /** @brief Committed orbit distance. */
    float _distance = 1.0f;
    /** @brief Committed camera rotation (the translation is not used). */
    aliceVision::geometry::Pose3 _pose;
    /** @brief Committed orbit center, in world coordinates. */
    Eigen::Vector3d _center = Eigen::Vector3d::Zero();
};
