#pragma once

#include <aliceVision/geometry/Pose3.hpp>

#include <Core/MotionInfo.hpp>

#include <QMatrix4x4>

/**
 * @brief MotionInfo backed by an explicit, QML-settable QMatrix4x4 pose.
 */
class AVMotionInfo : public MotionInfo
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief View (world to camera) matrix. */
    Q_PROPERTY(QMatrix4x4 pose READ pose WRITE setPose NOTIFY poseChanged)

  public:
    /**
     * @brief Build a motion with an identity pose.
     * @param[in] parent Optional Qt parent object.
     */
    explicit AVMotionInfo(QObject* parent = nullptr);

    /**
     * @brief Compare two motions by their view matrix.
     * @param[in] other Motion to compare with.
     * @return True if both matrices are equal.
     */
    bool operator==(const AVMotionInfo& other) const
    {
        return getMatrix() == other.getMatrix();
    }

    /** @brief Returns the pose as the view matrix. */
    QMatrix4x4 getMatrix() const override;

    /**
     * @brief The world-space pose matrix driving this motion info.
     * @return The current pose.
     */
    QMatrix4x4 pose() const
    {
        return _pose;
    }

    /**
     * @brief Sets the world-space pose matrix.
     * @param pose The new pose.
     */
    void setPose(const QMatrix4x4& pose)
    {
        if (_pose == pose)
        {
            return;
        }

        _pose = pose;
        emit poseChanged();
        emit changed();
    }

  signals:
    /** @brief Emitted when the pose changes. */
    void poseChanged();

  private:
    /** @brief View (world to camera) matrix. */
    QMatrix4x4 _pose;
};
