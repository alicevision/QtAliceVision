#pragma once

#include <aliceVision/geometry/Pose3.hpp>

#include <Core/MotionInfo.hpp>

class OrbitMotionInfo : public MotionInfo
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(float relativeRotationX READ relativeRotationX WRITE setRelativeRotationX NOTIFY relativeRotationXChanged)
    Q_PROPERTY(float relativeRotationY READ relativeRotationY WRITE setRelativeRotationY NOTIFY relativeRotationYChanged)
    Q_PROPERTY(float planeX READ planeX WRITE setPlaneX NOTIFY planeXChanged)
    Q_PROPERTY(float planeY READ planeY WRITE setPlaneY NOTIFY planeYChanged)
    Q_PROPERTY(float distance READ distance WRITE setDeltaDistance NOTIFY distanceChanged)

  public:
    explicit OrbitMotionInfo(QObject* parent = nullptr);

    Q_INVOKABLE void applyTransform();
    Q_INVOKABLE void setTransform(const QMatrix4x4& transform);
    Q_INVOKABLE void setCenter(const QVector3D& center);

    float relativeRotationX() const
    {
        return _relativeRotationX;
    }

    float relativeRotationY() const
    {
        return _relativeRotationY;
    }

    float planeX() const
    {
        return _planeX;
    }

    float planeY() const
    {
        return _planeY;
    }

    float distance() const
    {
        return _deltaDistance;
    }

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

    bool operator==(const OrbitMotionInfo& other) const
    {
        return getMatrix() == other.getMatrix();
    }

    QMatrix4x4 getMatrix() const override;
    Eigen::Matrix4d getEigenMatrix() const;
    Eigen::Vector3d getCenter() const override;

  signals:
    void relativeRotationXChanged();
    void relativeRotationYChanged();
    void planeXChanged();
    void planeYChanged();
    void distanceChanged();

  private:
    float _relativeRotationX = 0.0f;
    float _relativeRotationY = 0.0f;
    float _planeX = 0.0f;
    float _planeY = 0.0f;
    float _deltaDistance = 0.0f;

    float _distance = 1.0f;
    aliceVision::geometry::Pose3 _pose;
    Eigen::Vector3d _center = Eigen::Vector3d::Zero();
};
