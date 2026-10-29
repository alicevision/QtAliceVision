#pragma once

#include <Eigen/Core>

#include <QObject>
#include <QtQml/qqml.h>
#include <QMatrix4x4>

/**
 * @brief Base camera motion model providing the view (world to camera) transform of a SceneView.
 *
 * The default implementation is an identity transform centered on the origin. Subclasses
 * emit changed() whenever the transform is modified.
 */
class MotionInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT

  public:
    /**
     * @brief Build an identity motion.
     * @param[in] parent Optional Qt parent object.
     */
    explicit MotionInfo(QObject* parent = nullptr);
    /** @brief Destroy the motion. */
    ~MotionInfo() override;

    /**
     * @brief Get the view (world to camera) matrix.
     * @return Identity by default.
     */
    virtual QMatrix4x4 getMatrix() const;

    /**
     * @brief Get the point the camera motion is centered on, in world coordinates.
     * @return The origin by default.
     */
    virtual Eigen::Vector3d getCenter() const;

  signals:
    /** @brief Emitted when the view transform changes. */
    void changed();
};