#pragma once

#include <Core/CameraInfo.hpp>

class BaseCameraInfo : public CameraInfo
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(float fov READ fov WRITE setFov NOTIFY fovChanged)
    Q_PROPERTY(int imageWidth READ imageWidth WRITE setImageWidth NOTIFY imageWidthChanged)
    Q_PROPERTY(int imageHeight READ imageHeight WRITE setImageHeight NOTIFY imageHeightChanged)

  public:
    explicit BaseCameraInfo(QObject* parent = nullptr);

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

    int imageWidth() const
    {
        return _imageWidth;
    }

    void setImageWidth(int v)
    {
        if (_imageWidth == v)
        {
            return;
        }
        _imageWidth = v;
        emit imageWidthChanged();
        emit changed();
    }

    int imageHeight() const
    {
        return _imageHeight;
    }

    void setImageHeight(int v)
    {
        if (_imageHeight == v)
        {
            return;
        }
        _imageHeight = v;
        emit imageHeightChanged();
        emit changed();
    }

    bool operator==(const BaseCameraInfo& other) const;

  public:
    void setBackendProjectionMatrix(const QMatrix4x4& bpm);

    virtual QMatrix4x4 getProjectionMatrix(double aspectRatio) const;
    virtual QMatrix4x4 getOrthogonalMatrix(double aspectRatio) const;

  signals:
    void changed();
    void fovChanged();
    void imageWidthChanged();
    void imageHeightChanged();

  protected:
    QMatrix4x4 _backendProjectionMatrix;

    float _fov = 45.0f;
    int _imageWidth = 0;
    int _imageHeight = 0;
};