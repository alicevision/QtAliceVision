#pragma once

#include <Core/CameraInfo.hpp>

#include <algorithm>

class BaseCameraInfo : public CameraInfo
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(float fov READ fov WRITE setFov NOTIFY fovChanged)
    Q_PROPERTY(int imageWidth READ imageWidth WRITE setImageWidth NOTIFY imageWidthChanged)
    Q_PROPERTY(int imageHeight READ imageHeight WRITE setImageHeight NOTIFY imageHeightChanged)
    /** @brief When true, getProjectionMatrix() returns a Maya-like orthographic projection instead of a perspective one. */
    Q_PROPERTY(bool orthographic READ orthographic WRITE setOrthographic NOTIFY orthographicChanged)
    /** @brief Horizontal extent of the orthographic view, in world units (Maya's orthographicWidth). */
    Q_PROPERTY(float orthographicWidth READ orthographicWidth WRITE setOrthographicWidth NOTIFY orthographicWidthChanged)

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

    bool orthographic() const
    {
        return _orthographic;
    }

    void setOrthographic(bool v)
    {
        if (_orthographic == v)
        {
            return;
        }
        _orthographic = v;
        emit orthographicChanged();
        emit changed();
    }

    float orthographicWidth() const
    {
        return _orthographicWidth;
    }

    /**
     * @brief Set the orthographic view width.
     * @param v Width in world units, clamped to a small positive minimum.
     */
    void setOrthographicWidth(float v)
    {
        v = std::max(v, minOrthographicWidth);
        if (qFuzzyCompare(_orthographicWidth, v))
        {
            return;
        }
        _orthographicWidth = v;
        emit orthographicWidthChanged();
        emit changed();
    }

    bool operator==(const BaseCameraInfo& other) const;

  public:
    /**
     * @brief Build the projection matrix, including the backend clip-space correction.
     *
     * In perspective mode, uses fov, nearPlane and farPlane.
     * In orthographic mode (Maya-like, horizontal fit), the view spans orthographicWidth
     * world units horizontally and orthographicWidth / aspectRatio vertically.
     *
     * @param aspectRatio Viewport width / height.
     * @return The projection matrix.
     * @note In orthographic mode, geometry closer to the camera than nearPlane is still clipped,
     *       so keep the orbit distance large enough or nearPlane small enough.
     */
    virtual QMatrix4x4 getProjectionMatrix(double aspectRatio) const;
    virtual QMatrix4x4 getOrthogonalMatrix(double aspectRatio) const;

  signals:
    void fovChanged();
    void imageWidthChanged();
    void imageHeightChanged();
    void orthographicChanged();
    void orthographicWidthChanged();

  protected:
    /** @brief Lower bound for orthographicWidth, avoids a degenerate projection. */
    static constexpr float minOrthographicWidth = 1e-4f;

    float _fov = 45.0f;
    int _imageWidth = 0;
    int _imageHeight = 0;
    bool _orthographic = false;
    float _orthographicWidth = 30.0f;
};