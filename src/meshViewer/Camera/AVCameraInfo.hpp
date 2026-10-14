#pragma once

#include <Core/CameraInfo.hpp>

#include <QMatrix4x4>

class AVCameraInfo : public CameraInfo
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double sx READ sx WRITE setSx NOTIFY sxChanged)
    Q_PROPERTY(double sy READ sy WRITE setSy NOTIFY syChanged)
    Q_PROPERTY(double cx READ cx WRITE setCx NOTIFY cxChanged)
    Q_PROPERTY(double cy READ cy WRITE setCy NOTIFY cyChanged)
    Q_PROPERTY(int imageWidth READ imageWidth WRITE setImageWidth NOTIFY imageWidthChanged)
    Q_PROPERTY(int imageHeight READ imageHeight WRITE setImageHeight NOTIFY imageHeightChanged)
    Q_PROPERTY(float panX READ panX WRITE setPanX NOTIFY panXChanged)
    Q_PROPERTY(float panY READ panY WRITE setPanY NOTIFY panYChanged)
    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)

  public:
    explicit AVCameraInfo(QObject* parent = nullptr);

    double sx() const
    {
        return _sx;
    }

    void setSx(double v)
    {
        if (qFuzzyCompare(_sx, v))
        {
            return;
        }
        _sx = v;
        emit sxChanged();
        emit changed();
    }

    double sy() const
    {
        return _sy;
    }

    void setSy(double v)
    {
        if (qFuzzyCompare(_sy, v))
        {
            return;
        }
        _sy = v;
        emit syChanged();
        emit changed();
    }

    double cx() const
    {
        return _cx;
    }

    void setCx(double v)
    {
        if (qFuzzyCompare(_cx, v))
        {
            return;
        }
        _cx = v;
        emit cxChanged();
        emit changed();
    }

    double cy() const
    {
        return _cy;
    }

    void setCy(double v)
    {
        if (qFuzzyCompare(_cy, v))
        {
            return;
        }
        _cy = v;
        emit cyChanged();
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

    float panX() const
    {
        return _panX;
    }

    void setPanX(float v)
    {
        if (qFuzzyCompare(_panX, v))
        {
            return;
        }
        _panX = v;
        emit panXChanged();
        emit changed();
    }

    float panY() const
    {
        return _panY;
    }

    void setPanY(float v)
    {
        if (qFuzzyCompare(_panY, v))
        {
            return;
        }
        _panY = v;
        emit panYChanged();
        emit changed();
    }

    float zoom() const
    {
        return _zoom;
    }

    void setZoom(float v)
    {
        if (qFuzzyCompare(_zoom, v))
        {
            return;
        }
        _zoom = v;
        emit zoomChanged();
        emit changed();
    }

    bool operator==(const AVCameraInfo& other) const;

    QMatrix4x4 getProjectionMatrix(double aspectRatio) const override;
    QMatrix4x4 getOrthogonalMatrix(double aspectRatio) const override;

  signals:

    void sxChanged();
    void syChanged();
    void cxChanged();
    void cyChanged();
    void imageWidthChanged();
    void imageHeightChanged();
    void panXChanged();
    void panYChanged();
    void zoomChanged();

  private:
    int _imageWidth = 0;
    int _imageHeight = 0;
    double _sx = 1.0;
    double _sy = 1.0;
    double _cx = 0.0;
    double _cy = 0.0;
    float _panX = 0.0f;
    float _panY = 0.0f;
    float _zoom = 1.0f;
};
