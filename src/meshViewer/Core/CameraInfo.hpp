#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QMatrix4x4>

class CameraInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(float fov READ fov WRITE setFov NOTIFY fovChanged)
    Q_PROPERTY(float nearPlane READ nearPlane WRITE setNearPlane NOTIFY nearPlaneChanged)
    Q_PROPERTY(float farPlane READ farPlane WRITE setFarPlane NOTIFY farPlaneChanged)

  public:
    explicit CameraInfo(QObject* parent = nullptr);
    ~CameraInfo() override;

  public:
    float fov() const
    {
        return _fov;
    }

    void setFov(float v)
    {
        if (qFuzzyCompare(_fov, v))
        {
            return;
        }

        _fov = v;

        emit fovChanged();
        emit changed();
    }

    float nearPlane() const
    {
        return _nearPlane;
    }

    void setNearPlane(float v)
    {
        if (qFuzzyCompare(_nearPlane, v))
        {
            return;
        }

        _nearPlane = v;

        emit nearPlaneChanged();
        emit changed();
    }

    float farPlane() const
    {
        return _farPlane;
    }

    void setFarPlane(float v)
    {
        if (qFuzzyCompare(_farPlane, v))
        {
            return;
        }

        _farPlane = v;

        emit farPlaneChanged();
        emit changed();
    }

  public:
    void setBackendProjectionMatrix(const QMatrix4x4& bpm);

    virtual QMatrix4x4 getProjectionMatrix(double aspectRatio) const;
    virtual QMatrix4x4 getOrthogonalMatrix(double aspectRatio) const;

  signals:
    void changed();
    void fovChanged();
    void nearPlaneChanged();
    void farPlaneChanged();

  protected:
    QMatrix4x4 _backendProjectionMatrix;

    float _fov = 45.0f;
    float _nearPlane = 0.001f;
    float _farPlane = 100.0f;
};