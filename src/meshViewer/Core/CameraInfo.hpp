#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QMatrix4x4>

/**
 * @brief Abstract camera model providing the projection used to render a SceneView.
 *
 * Concrete subclasses describe the camera intrinsics and implement getProjectionMatrix()
 * and getOrthogonalMatrix(). Any parameter change emits changed() so the view can redraw.
 */
class CameraInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("CameraInfo is an abstract base class")

    /** @brief Distance to the near clipping plane, in world units. */
    Q_PROPERTY(float nearPlane READ nearPlane WRITE setNearPlane NOTIFY nearPlaneChanged)
    /** @brief Distance to the far clipping plane, in world units. */
    Q_PROPERTY(float farPlane READ farPlane WRITE setFarPlane NOTIFY farPlaneChanged)

  public:
    /**
     * @brief Build a camera with default clipping planes.
     * @param[in] parent Optional Qt parent object.
     */
    explicit CameraInfo(QObject* parent = nullptr);
    /** @brief Destroy the camera. */
    ~CameraInfo() override;

  public:
    /** @brief Get the distance to the near clipping plane. */
    float nearPlane() const
    {
        return _nearPlane;
    }

    /**
     * @brief Set the distance to the near clipping plane.
     * @param[in] v Distance, in world units.
     */
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

    /** @brief Get the distance to the far clipping plane. */
    float farPlane() const
    {
        return _farPlane;
    }

    /**
     * @brief Set the distance to the far clipping plane.
     * @param[in] v Distance, in world units.
     */
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
    /**
     * @brief Set the clip-space correction matrix of the rendering backend.
     *
     * Projection matrices are built with OpenGL conventions; this matrix (typically
     * QRhi::clipSpaceCorrMatrix()) adapts them to the active graphics API.
     *
     * @param[in] bpm Correction matrix premultiplied to the projection matrix.
     */
    void setBackendProjectionMatrix(const QMatrix4x4& bpm);

    /**
     * @brief Build the perspective projection matrix for a viewport.
     * @param[in] aspectRatio Aspect ratio (width / height) of the viewport.
     * @return The projection matrix, including the backend correction.
     */
    virtual QMatrix4x4 getProjectionMatrix(double aspectRatio) const = 0;
    /**
     * @brief Build the 2D NDC transform used to draw screen-aligned content (e.g. the camera image).
     * @param[in] aspectRatio Aspect ratio (width / height) of the viewport.
     * @return The 2D transform matrix.
     */
    virtual QMatrix4x4 getOrthogonalMatrix(double aspectRatio) const = 0;

  signals:
    /** @brief Emitted when any parameter affecting the camera matrices changes. */
    void changed();
    /** @brief Emitted when the near clipping plane changes. */
    void nearPlaneChanged();
    /** @brief Emitted when the far clipping plane changes. */
    void farPlaneChanged();

  protected:
    /** @brief Clip-space correction matrix of the rendering backend. */
    QMatrix4x4 _backendProjectionMatrix;

    /** @brief Distance to the near clipping plane. */
    float _nearPlane = 0.001f;
    /** @brief Distance to the far clipping plane. */
    float _farPlane = 100.0f;
};