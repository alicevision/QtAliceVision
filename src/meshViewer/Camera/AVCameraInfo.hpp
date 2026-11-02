#pragma once

#include <Core/CameraInfo.hpp>

#include <QMatrix4x4>

/**
 * @brief Camera described by AliceVision pinhole intrinsics, exposed to QML.
 *
 * Intrinsics are expressed in pixels in the image frame (origin at the top-left corner, Y pointing down).
 * In addition to the intrinsics, the camera holds a 2D zoom and pan applied in NDC, so that the
 * projected scene and the camera image can be navigated together.
 *
 * Every setter emits its dedicated notification signal followed by CameraInfo::changed().
 */
class AVCameraInfo : public CameraInfo
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Horizontal focal length, in pixels. */
    Q_PROPERTY(double sx READ sx WRITE setSx NOTIFY sxChanged)
    /** @brief Vertical focal length, in pixels. */
    Q_PROPERTY(double sy READ sy WRITE setSy NOTIFY syChanged)
    /** @brief Horizontal coordinate of the principal point, in pixels. */
    Q_PROPERTY(double cx READ cx WRITE setCx NOTIFY cxChanged)
    /** @brief Vertical coordinate of the principal point, in pixels. */
    Q_PROPERTY(double cy READ cy WRITE setCy NOTIFY cyChanged)
    /** @brief Image width, in pixels. A non-positive value disables image fitting. */
    Q_PROPERTY(int imageWidth READ imageWidth WRITE setImageWidth NOTIFY imageWidthChanged)
    /** @brief Image height, in pixels. A non-positive value disables image fitting. */
    Q_PROPERTY(int imageHeight READ imageHeight WRITE setImageHeight NOTIFY imageHeightChanged)
    /** @brief Horizontal pan offset, in NDC units. */
    Q_PROPERTY(float panX READ panX WRITE setPanX NOTIFY panXChanged)
    /** @brief Vertical pan offset, in NDC units. */
    Q_PROPERTY(float panY READ panY WRITE setPanY NOTIFY panYChanged)
    /** @brief Zoom factor applied in NDC (1 means the image fits the viewport). */
    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)

  public:
    explicit AVCameraInfo(QObject* parent = nullptr);

    /** @brief Get the horizontal focal length, in pixels. */
    double sx() const
    {
        return _sx;
    }

    /**
     * @brief Set the horizontal focal length.
     * @param[in] v Focal length, in pixels.
     */
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

    /** @brief Get the vertical focal length, in pixels. */
    double sy() const
    {
        return _sy;
    }

    /**
     * @brief Set the vertical focal length.
     * @param[in] v Focal length, in pixels.
     */
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

    /** @brief Get the horizontal coordinate of the principal point, in pixels. */
    double cx() const
    {
        return _cx;
    }

    /**
     * @brief Set the horizontal coordinate of the principal point.
     * @param[in] v Coordinate, in pixels.
     */
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

    /** @brief Get the vertical coordinate of the principal point, in pixels. */
    double cy() const
    {
        return _cy;
    }

    /**
     * @brief Set the vertical coordinate of the principal point.
     * @param[in] v Coordinate, in pixels.
     */
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

    /** @brief Get the image width, in pixels. */
    int imageWidth() const
    {
        return _imageWidth;
    }

    /**
     * @brief Set the image width.
     * @param[in] v Width, in pixels.
     */
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

    /** @brief Get the image height, in pixels. */
    int imageHeight() const
    {
        return _imageHeight;
    }

    /**
     * @brief Set the image height.
     * @param[in] v Height, in pixels.
     */
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

    /** @brief Get the horizontal pan offset, in NDC units. */
    float panX() const
    {
        return _panX;
    }

    /**
     * @brief Set the horizontal pan offset.
     * @param[in] v Offset, in NDC units.
     */
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

    /** @brief Get the vertical pan offset, in NDC units. */
    float panY() const
    {
        return _panY;
    }

    /**
     * @brief Set the vertical pan offset.
     * @param[in] v Offset, in NDC units.
     */
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

    /** @brief Get the zoom factor. */
    float zoom() const
    {
        return _zoom;
    }

    /**
     * @brief Set the zoom factor.
     * @param[in] v Zoom factor (1 means the image fits the viewport).
     */
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

    /** @brief Emitted when the horizontal focal length changes. */
    void sxChanged();
    /** @brief Emitted when the vertical focal length changes. */
    void syChanged();
    /** @brief Emitted when the horizontal principal point coordinate changes. */
    void cxChanged();
    /** @brief Emitted when the vertical principal point coordinate changes. */
    void cyChanged();
    /** @brief Emitted when the image width changes. */
    void imageWidthChanged();
    /** @brief Emitted when the image height changes. */
    void imageHeightChanged();
    /** @brief Emitted when the horizontal pan offset changes. */
    void panXChanged();
    /** @brief Emitted when the vertical pan offset changes. */
    void panYChanged();
    /** @brief Emitted when the zoom factor changes. */
    void zoomChanged();

  private:
    /** @brief Image width, in pixels. */
    int _imageWidth = 0;
    /** @brief Image height, in pixels. */
    int _imageHeight = 0;
    /** @brief Horizontal focal length, in pixels. */
    double _sx = 1.0;
    /** @brief Vertical focal length, in pixels. */
    double _sy = 1.0;
    /** @brief Horizontal principal point coordinate, in pixels. */
    double _cx = 0.0;
    /** @brief Vertical principal point coordinate, in pixels. */
    double _cy = 0.0;
    /** @brief Horizontal pan offset, in NDC units. */
    float _panX = 0.0f;
    /** @brief Vertical pan offset, in NDC units. */
    float _panY = 0.0f;
    /** @brief Zoom factor applied in NDC. */
    float _zoom = 1.0f;
};
