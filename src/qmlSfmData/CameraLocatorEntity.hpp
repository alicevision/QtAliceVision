#pragma once

#include <QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DCore/QAttribute>

#include <Eigen/Dense>

#include <aliceVision/types.hpp>

namespace sfmdataentity {

class CameraLocatorEntity : public Qt3DCore::QEntity
{
    Q_OBJECT

    Q_PROPERTY(quint32 viewId MEMBER _viewId NOTIFY viewIdChanged)
    Q_PROPERTY(quint32 resectionId MEMBER _resectionId NOTIFY resectionIdChanged)

  public:
    explicit CameraLocatorEntity(const aliceVision::IndexT& viewId, const aliceVision::IndexT& resectionId,
                                 float hfov, float vfov, Qt3DCore::QNode* = nullptr);
    ~CameraLocatorEntity() override = default;

    void setTransform(const Eigen::Matrix4d&);
    QVector<float> initializeColors(int size, float defaultValue = 1.0f);
    void updateColors(float red, float green, float blue);

    aliceVision::IndexT viewId() const { return _viewId; }
    aliceVision::IndexT resectionId() const { return _resectionId; }

    Q_SIGNAL void viewIdChanged();
    Q_SIGNAL void resectionIdChanged();

  private:
    Qt3DCore::QTransform* _transform;
    aliceVision::IndexT _viewId;
    aliceVision::IndexT _resectionId;
    Qt3DCore::QAttribute* _colorAttribute;
    QVector<float> _colors;
};

}  // namespace sfmdataentity
