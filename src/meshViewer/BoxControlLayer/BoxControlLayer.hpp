#pragma once

#include <Core/LayerItem.hpp>
#include <Core/SceneView.hpp>

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QVector3D>

/**
 * @brief Layer that renders an oriented box together with a transform gizmo.
 *
 * The box is the unit cube [-1, 1]^3 transformed by translation * rotation * scale.
 * The gizmo is drawn at the box center, aligned with the box axes, on top of the scene, and keeps
 * a constant screen size. It provides translate (cones), rotate (rings), scale (cubes) and
 * face (spheres) handles, one per axis. Translate and face handles are placed outside the box,
 * at a distance that follows the box size; handle sizes stay constant on screen.
 *
 * This layer only draws and provides geometric queries (handleAt(), axisDirection(), gizmoWorldScale()).
 * Interactions (mouse, keyboard) are left to the application.
 */
class BoxControlLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Box center, in world coordinates. */
    Q_PROPERTY(QVector3D translation READ translation WRITE setTranslation NOTIFY translationChanged)
    /** @brief Box orientation as Euler angles in degrees (QQuaternion::fromEulerAngles convention). */
    Q_PROPERTY(QVector3D rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    /** @brief Box half extents along its local axes. */
    Q_PROPERTY(QVector3D scale READ scale WRITE setScale NOTIFY scaleChanged)
    /** @brief Whether the gizmo is drawn. */
    Q_PROPERTY(bool gizmoVisible READ gizmoVisible WRITE setGizmoVisible NOTIFY gizmoVisibleChanged)
    /** @brief Gizmo size, as a fraction of the viewport half height. */
    Q_PROPERTY(float gizmoSize READ gizmoSize WRITE setGizmoSize NOTIFY gizmoSizeChanged)
    /** @brief Handle drawn highlighted (see Handle), or NoHandle. */
    Q_PROPERTY(int highlightedHandle READ highlightedHandle WRITE setHighlightedHandle NOTIFY highlightedHandleChanged)

  public:
    /** @brief Interactive handles of the gizmo. */
    enum Handle
    {
        NoHandle = 0, /**< No handle. */
        TranslateX,   /**< Arrow translating along the box X axis. */
        TranslateY,   /**< Arrow translating along the box Y axis. */
        TranslateZ,   /**< Arrow translating along the box Z axis. */
        RotateX,      /**< Ring rotating around the box X axis. */
        RotateY,      /**< Ring rotating around the box Y axis. */
        RotateZ,      /**< Ring rotating around the box Z axis. */
        ScaleX,       /**< Cube scaling the box symmetrically along X. */
        ScaleY,       /**< Cube scaling the box symmetrically along Y. */
        ScaleZ,       /**< Cube scaling the box symmetrically along Z. */
        FaceXPos,     /**< Sphere moving the +X face only. */
        FaceXNeg,     /**< Sphere moving the -X face only. */
        FaceYPos,     /**< Sphere moving the +Y face only. */
        FaceYNeg,     /**< Sphere moving the -Y face only. */
        FaceZPos,     /**< Sphere moving the +Z face only. */
        FaceZNeg      /**< Sphere moving the -Z face only. */
    };
    Q_ENUM(Handle)

    /**
     * @brief Build a unit box at the origin.
     * @param[in] parent Optional Qt parent object.
     */
    explicit BoxControlLayer(QObject* parent = nullptr);

    /** @brief Get the box center, in world coordinates. */
    QVector3D translation() const
    {
        return _translation;
    }
    /**
     * @brief Set the box center.
     * @param[in] translation Center, in world coordinates.
     */
    void setTranslation(const QVector3D& translation);

    /** @brief Get the box orientation as Euler angles, in degrees. */
    QVector3D rotation() const
    {
        return _rotation;
    }
    /**
     * @brief Set the box orientation.
     * @param[in] rotation Euler angles, in degrees.
     */
    void setRotation(const QVector3D& rotation);

    /** @brief Get the box half extents. */
    QVector3D scale() const
    {
        return _scale;
    }
    /**
     * @brief Set the box half extents.
     * @param[in] scale Half extents along the box local axes.
     */
    void setScale(const QVector3D& scale);

    /** @brief Returns true when the gizmo is drawn. */
    bool gizmoVisible() const
    {
        return _gizmoVisible;
    }
    /**
     * @brief Show or hide the gizmo.
     * @param[in] gizmoVisible True to draw the gizmo.
     */
    void setGizmoVisible(bool gizmoVisible);

    /** @brief Get the gizmo size, as a fraction of the viewport half height. */
    float gizmoSize() const
    {
        return _gizmoSize;
    }
    /**
     * @brief Set the gizmo size.
     * @param[in] gizmoSize Fraction of the viewport half height.
     */
    void setGizmoSize(float gizmoSize);

    /** @brief Get the highlighted handle. */
    int highlightedHandle() const
    {
        return _highlightedHandle;
    }
    /**
     * @brief Set the highlighted handle.
     * @param[in] handle A Handle value.
     */
    void setHighlightedHandle(int handle);

    /** @brief Returns the box orientation as a quaternion. */
    QQuaternion orientation() const
    {
        return QQuaternion::fromEulerAngles(_rotation);
    }

    /** @brief Returns the box model matrix (translation * rotation * scale). */
    QMatrix4x4 boxMatrix() const;

    /**
     * @brief Returns the world size of one gizmo unit, so that the gizmo keeps a constant screen size.
     * Shared by the renderable and handleAt() so that what is drawn matches what is picked.
     * @param viewProjection Current view-projection matrix.
     * @param projectionScaleY Vertical scale of the projection matrix (element (1, 1)).
     * @param center Gizmo center, in world coordinates.
     * @param gizmoSize Gizmo size, as a fraction of the viewport half height.
     */
    static float gizmoScaleFor(const QMatrix4x4& viewProjection, float projectionScaleY, const QVector3D& center, float gizmoSize);

    /**
     * @brief Returns the world position of a face handle.
     * @param axis Box axis (0 = X, 1 = Y, 2 = Z).
     * @param sign +1 for the positive face, -1 for the negative one.
     */
    QVector3D facePosition(int axis, float sign) const;

    /**
     * @brief Returns the gizmo handle under a screen position.
     * @param view SceneView providing the camera.
     * @param mousePos Position in @p view coordinates (pixels).
     * @return The closest handle hit (see Handle), or NoHandle.
     */
    Q_INVOKABLE int handleAt(SceneView* view, const QVector2D& mousePos) const;

    /** @brief Returns the world direction of the box local axis @p axis (0 = X, 1 = Y, 2 = Z). */
    Q_INVOKABLE QVector3D axisDirection(int axis) const;

    /** @brief Returns the world size of one gizmo unit for the current camera of @p view. */
    Q_INVOKABLE float gizmoWorldScale(SceneView* view) const;

    /** @brief Returns the axis (0 = X, 1 = Y, 2 = Z) of @p handle, or -1 for NoHandle. */
    Q_INVOKABLE static int handleAxis(int handle);

    /** @brief Returns +1 for positive face handles, -1 for negative ones and 0 for other handles. */
    Q_INVOKABLE static int handleSign(int handle);

    /** @brief Returns true when @p handle is a translate handle. */
    Q_INVOKABLE static bool isTranslateHandle(int handle)
    {
        return handle >= TranslateX && handle <= TranslateZ;
    }
    /** @brief Returns true when @p handle is a rotate handle. */
    Q_INVOKABLE static bool isRotateHandle(int handle)
    {
        return handle >= RotateX && handle <= RotateZ;
    }
    /** @brief Returns true when @p handle is a scale handle. */
    Q_INVOKABLE static bool isScaleHandle(int handle)
    {
        return handle >= ScaleX && handle <= ScaleZ;
    }
    /** @brief Returns true when @p handle is a face handle. */
    Q_INVOKABLE static bool isFaceHandle(int handle)
    {
        return handle >= FaceXPos && handle <= FaceZNeg;
    }

    /** @brief Returns the world bounds of the oriented box. */
    BoundingBox boundingBox() const override;

    /** @brief The box and gizmo are drawn on top of the scene. */
    bool rendersInForeground() const override
    {
        return true;
    }

    /** @brief Create a BoxControlRenderable. */
    std::unique_ptr<IRenderable> createRenderable() const override;

  signals:
    /** @brief Emitted when the box center changes. */
    void translationChanged();
    /** @brief Emitted when the box orientation changes. */
    void rotationChanged();
    /** @brief Emitted when the box half extents change. */
    void scaleChanged();
    /** @brief Emitted when the gizmo visibility changes. */
    void gizmoVisibleChanged();
    /** @brief Emitted when the gizmo size changes. */
    void gizmoSizeChanged();
    /** @brief Emitted when the highlighted handle changes. */
    void highlightedHandleChanged();

  private:
    /** @brief Box center. */
    QVector3D _translation;
    /** @brief Box orientation, as Euler angles in degrees. */
    QVector3D _rotation;
    /** @brief Box half extents. */
    QVector3D _scale = QVector3D(1.f, 1.f, 1.f);
    /** @brief Gizmo visibility. */
    bool _gizmoVisible = true;
    /** @brief Gizmo size, as a fraction of the viewport half height. */
    float _gizmoSize = 0.15f;
    /** @brief Highlighted handle. */
    int _highlightedHandle = NoHandle;
};
