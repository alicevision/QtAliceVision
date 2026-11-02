#pragma once

#include <Core/CameraInfo.hpp>

#include <algorithm>

/**
 * @brief Interactive viewer camera with a vertical field of view, optionally orthographic.
 *
 * Unlike AVCameraInfo, the projection does not depend on image intrinsics; the image size is
 * only used to fit screen-aligned content in getOrthogonalMatrix().
 */
class BaseCameraInfo : public CameraInfo
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Vertical field of view, in degrees. */
    Q_PROPERTY(float fov READ fov WRITE setFov NOTIFY fovChanged)
    /** @brief Width of the displayed image, in pixels. A non-positive value disables image fitting. */
    Q_PROPERTY(int imageWidth READ imageWidth WRITE setImageWidth NOTIFY imageWidthChanged)
    /** @brief Height of the displayed image, in pixels. A non-positive value disables image fitting. */
    Q_PROPERTY(int imageHeight READ imageHeight WRITE setImageHeight NOTIFY imageHeightChanged)
    /** @brief When true, getProjectionMatrix() returns a Maya-like orthographic projection instead of a perspective one. */
    Q_PROPERTY(bool orthographic READ orthographic WRITE setOrthographic NOTIFY orthographicChanged)
    /** @brief Horizontal extent of the orthographic view, in world units (Maya's orthographicWidth). */
    Q_PROPERTY(float orthographicWidth READ orthographicWidth WRITE setOrthographicWidth NOTIFY orthographicWidthChanged)

  public:
    /**
     * @brief Build a perspective camera with default parameters.
     * @param[in] parent Optional Qt parent object.
     */
    explicit BaseCameraInfo(QObject* parent = nullptr);

  public:
    /** @brief Get the vertical field of view, in degrees. */
    float fov() const
    {
        return _fov;
    }

    /**
     * @brief Set the vertical field of view.
     * @param[in] v Field of view, in degrees.
     */
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

    /** @brief Returns true when the orthographic projection is used. */
    bool orthographic() const
    {
        return _orthographic;
    }

    /**
     * @brief Switch between perspective and orthographic projection.
     * @param[in] v True for orthographic.
     */
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

    /** @brief Get the horizontal extent of the orthographic view, in world units. */
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

    /**
     * @brief Compare two cameras on their projection parameters.
     * @param[in] other Camera to compare with.
     * @return True if all parameters are exactly equal.
     * @note nearPlane is not part of the comparison.
     */
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
    /**
     * @brief Build the 2D NDC scale fitting the image inside the viewport without distortion.
     * @param[in] aspectRatio Viewport width / height.
     * @return The scale matrix, or identity if the image size or @p aspectRatio is not strictly positive.
     */
    virtual QMatrix4x4 getOrthogonalMatrix(double aspectRatio) const;

  signals:
    /** @brief Emitted when the field of view changes. */
    void fovChanged();
    /** @brief Emitted when the image width changes. */
    void imageWidthChanged();
    /** @brief Emitted when the image height changes. */
    void imageHeightChanged();
    /** @brief Emitted when the projection mode changes. */
    void orthographicChanged();
    /** @brief Emitted when the orthographic width changes. */
    void orthographicWidthChanged();

  protected:
    /** @brief Lower bound for orthographicWidth, avoids a degenerate projection. */
    static constexpr float minOrthographicWidth = 1e-4f;

    /** @brief Vertical field of view, in degrees. */
    float _fov = 45.0f;
    /** @brief Image width, in pixels. */
    int _imageWidth = 0;
    /** @brief Image height, in pixels. */
    int _imageHeight = 0;
    /** @brief Orthographic projection flag. */
    bool _orthographic = false;
    /** @brief Horizontal extent of the orthographic view, in world units. */
    float _orthographicWidth = 30.0f;
};