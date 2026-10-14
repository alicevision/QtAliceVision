#pragma once

#include <Core/CameraInfo.hpp>

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

    bool operator==(const BaseCameraInfo& other) const;

  public:
    void setBackendProjectionMatrix(const QMatrix4x4& bpm);

    virtual QMatrix4x4 getProjectionMatrix(double aspectRatio) const;
    /**
     * @brief Build the 2D NDC scale fitting the image inside the viewport without distortion.
     * @param[in] aspectRatio Viewport width / height.
     * @return The scale matrix, or identity if the image size or @p aspectRatio is not strictly positive.
     */
    virtual QMatrix4x4 getOrthogonalMatrix(double aspectRatio) const;

  signals:
    void changed();
    /** @brief Emitted when the field of view changes. */
    void fovChanged();
    /** @brief Emitted when the image width changes. */
    void imageWidthChanged();
    /** @brief Emitted when the image height changes. */
    void imageHeightChanged();

  protected:
    QMatrix4x4 _backendProjectionMatrix;

    /** @brief Vertical field of view, in degrees. */
    float _fov = 45.0f;
    /** @brief Image width, in pixels. */
    int _imageWidth = 0;
    /** @brief Image height, in pixels. */
    int _imageHeight = 0;
};