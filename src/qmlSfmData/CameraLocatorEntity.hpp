#pragma once

#include <QEntity>
#include <Qt3DCore/QTransform>
#include <Eigen/Dense>
#include <aliceVision/types.hpp>

namespace sfmdataentity {

class CameraLocatorEntity : public Qt3DCore::QEntity
{
    Q_OBJECT

    Q_PROPERTY(quint32 viewId MEMBER _viewId NOTIFY viewIdChanged)

  public:
    explicit CameraLocatorEntity(const aliceVision::IndexT& viewId, float hfov, float vfov, Qt3DCore::QNode* = nullptr);
    ~CameraLocatorEntity() override = default;

    void setTransform(const Eigen::Matrix4d&);

    Q_SIGNAL void viewIdChanged();

  private:
    Qt3DCore::QTransform* _transform;
    aliceVision::IndexT _viewId;
};

}  // namespace sfmdataentity
