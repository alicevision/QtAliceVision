#pragma once

#include <QEntity>
#include <QUrl>

#include <Qt3DCore/QTransform>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QMaterial>
#include <QQmlListProperty>

#include <iostream>

#include <aliceVision/types.hpp>

namespace sfmdataentity {
class PointCloudEntity;
class CameraLocatorEntity;
class IOThread;

class SfmDataEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool skipHidden MEMBER _skipHidden NOTIFY skipHiddenChanged)
    Q_PROPERTY(float pointSize READ pointSize WRITE setPointSize NOTIFY pointSizeChanged)
    Q_PROPERTY(float locatorScale READ locatorScale WRITE setLocatorScale NOTIFY locatorScaleChanged)
    Q_PROPERTY(QQmlListProperty<sfmdataentity::CameraLocatorEntity> cameras READ cameras NOTIFY camerasChanged)
    Q_PROPERTY(QQmlListProperty<sfmdataentity::PointCloudEntity> pointClouds READ pointClouds NOTIFY pointCloudsChanged)
    Q_PROPERTY(quint32 selectedViewId READ selectedViewId WRITE setSelectedViewId NOTIFY selectedViewIdChanged)
    Q_PROPERTY(quint32 resectionId READ resectionId WRITE setResectionId NOTIFY resectionIdChanged)
    Q_PROPERTY(bool displayResections READ displayResections WRITE setDisplayResections NOTIFY displayResectionsChanged)

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

  public:
    // Identical to SceneLoader.Status
    enum Status
    {
        None = 0,
        Loading,
        Ready,
        Error
    };
    Q_ENUM(Status)

    explicit SfmDataEntity(Qt3DCore::QNode* = nullptr);
    ~SfmDataEntity() override = default;

    Q_SLOT const QUrl& source() const { return _source; }
    Q_SLOT float pointSize() const { return _pointSize; }
    Q_SLOT float locatorScale() const { return _locatorScale; }
    Q_SLOT aliceVision::IndexT selectedViewId() const { return _selectedViewId; }
    Q_SLOT aliceVision::IndexT resectionId() const { return _resectionId; }
    Q_SLOT bool displayResections() const { return _displayResections; }
    Q_SLOT void setSource(const QUrl& source);
    Q_SLOT void setPointSize(const float& value);
    Q_SLOT void setLocatorScale(const float& value);
    Q_SLOT void setSelectedViewId(const aliceVision::IndexT& viewId);
    Q_SLOT void setResectionId(const aliceVision::IndexT& value);
    Q_SLOT void setDisplayResections(const bool value);

    Status status() const { return _status; }

    void setStatus(Status status)
    {
        if (status == _status)
            return;
        _status = status;
        Q_EMIT statusChanged(_status);
    }

    Q_SIGNAL void sourceChanged();
    Q_SIGNAL void camerasChanged();
    Q_SIGNAL void pointSizeChanged();
    Q_SIGNAL void pointCloudsChanged();
    Q_SIGNAL void locatorScaleChanged();
    Q_SIGNAL void objectPicked(Qt3DCore::QTransform* transform);
    Q_SIGNAL void statusChanged(Status status);
    Q_SIGNAL void skipHiddenChanged();
    Q_SIGNAL void selectedViewIdChanged();
    Q_SIGNAL void resectionIdChanged();
    Q_SIGNAL void displayResectionsChanged();

  protected:
    /// Scale child locators
    void scaleLocators() const;

    void onIOThreadFinished();

  private:
    /// Delete all child entities/components
    void clear();
    void loadSfmData();
    void createMaterials();

    QQmlListProperty<CameraLocatorEntity> cameras() { return {this, &_cameras}; }

    QQmlListProperty<PointCloudEntity> pointClouds() { return {this, &_pointClouds}; }

    Status _status = SfmDataEntity::None;
    QUrl _source;
    bool _skipHidden = false;
    float _pointSize = 0.5f;
    float _locatorScale = 1.0f;
    aliceVision::IndexT _selectedViewId = 0;
    aliceVision::IndexT _resectionId = 0;
    bool _displayResections = false;
    Qt3DRender::QParameter* _pointSizeParameter;
    Qt3DRender::QMaterial* _cloudMaterial;
    Qt3DRender::QMaterial* _cameraMaterial;
    QList<CameraLocatorEntity*> _cameras;
    QList<PointCloudEntity*> _pointClouds;
    std::unique_ptr<IOThread> _ioThread;
};

}  // namespace sfmdataentity
