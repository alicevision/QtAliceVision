#pragma once

#include <Core/BoundingBox.hpp>

#include <QMatrix4x4>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector3D>
#include <memory>
#include <unordered_map>
#include <vector>
#include <qqml.h>
#include <QFutureWatcher>

#include <aliceVision/types.hpp>
#include <aliceVision/geometry/Pose3.hpp>
#include <aliceVision/camera/IntrinsicBase.hpp>

class CameraInfo;

const float cameraDepth = 0.02f;
const float sphereRadius = 0.05f;

struct SfmDataPointInstance
{
    QVector3D position;
    QVector3D color;
};

struct SfmDataCameraInstance
{
    aliceVision::IndexT viewId = 0;
    aliceVision::IndexT frameId = 0;
    aliceVision::IndexT resectionId = 0;
    QString path = "";

    aliceVision::geometry::Pose3 camera_T_world;
    std::shared_ptr<aliceVision::camera::IntrinsicBase> intrinsics;
};

QMatrix4x4 toQMatrix4x4(const aliceVision::geometry::Pose3& pose);

struct SfmDataContent
{
    std::vector<SfmDataPointInstance> points;
    std::vector<SfmDataCameraInstance> cameras;
    std::unordered_map<aliceVision::IndexT, SfmDataCameraInstance> cameraPerViewId;

    quint32 maxResectionId = 0;
    QString errorString;
    bool valid = false;

    BoundingBox pointCloudBoundingBox;   /**< Bounds of the landmark positions; empty when there are none. */
    BoundingBox cameraCenterBoundingBox; /**< Bounds of the camera centers; empty when there are none. */
};

class SfmDataObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)
    Q_PROPERTY(int pointCount READ pointCount NOTIFY pointCountChanged)
    Q_PROPERTY(int cameraCount READ cameraCount NOTIFY cameraCountChanged)
    Q_PROPERTY(quint32 maxResectionId READ maxResectionId NOTIFY maxResectionIdChanged)
    Q_PROPERTY(quint32 limitResectionId READ limitResectionId WRITE setLimitResectionId NOTIFY limitResectionIdChanged)
    Q_PROPERTY(QVariantList pointCloudBoundingBox READ pointCloudBoundingBox NOTIFY pointCloudBoundingBoxChanged)
    Q_PROPERTY(QVariantList cameraCenterBoundingBox READ cameraCenterBoundingBox NOTIFY cameraCenterBoundingBoxChanged)

  public:
    explicit SfmDataObject(QObject* parent = nullptr);
    ~SfmDataObject() override;

    QString source() const
    {
        return _source;
    }
    void setSource(const QString& path);

    bool loading() const
    {
        return _loading;
    }
    QString errorString() const
    {
        return _errorString;
    }
    bool valid() const
    {
        return _content && _content->valid;
    }
    int pointCount() const
    {
        return _content ? static_cast<int>(_content->points.size()) : 0;
    }
    int cameraCount() const
    {
        return _content ? static_cast<int>(_content->cameraPerViewId.size()) : 0;
    }
    quint32 maxResectionId() const
    {
        return _content ? _content->maxResectionId : 0;
    }
    quint32 limitResectionId() const
    {
        return _limitResectionId;
    }
    void setLimitResectionId(quint32 value);

    QVariantList pointCloudBoundingBox() const
    {
        QVariantList box;
        if (!_content || !_content->pointCloudBoundingBox.isValid())
        {
            return box;
        }
        box << QVariant::fromValue(_content->pointCloudBoundingBox.min());
        box << QVariant::fromValue(_content->pointCloudBoundingBox.max());
        return box;
    }

    QVariantList cameraCenterBoundingBox() const
    {
        QVariantList box;
        if (!_content || !_content->cameraCenterBoundingBox.isValid())
        {
            return box;
        }
        box << QVariant::fromValue(_content->cameraCenterBoundingBox.min());
        box << QVariant::fromValue(_content->cameraCenterBoundingBox.max());
        return box;
    }

    /** @brief C++ API for layers — call only after dataReady() has been emitted. */
    const SfmDataContent& content() const
    {
        return *_content;
    }

    /** @brief Whether a camera transform is available for the given view id. */
    Q_INVOKABLE bool hasCameraTransform(quint32 viewId) const;
    Q_INVOKABLE QMatrix4x4 getCameraTransform(quint32 viewId) const;
    Q_INVOKABLE QVector3D getCameraCenter(quint32 viewId) const;
    Q_INVOKABLE QString getImagePath(quint32 viewId) const;
    Q_INVOKABLE CameraInfo* getCameraInfo(quint32 viewId) const;

  signals:
    void sourceChanged();
    void loadingChanged();
    void errorStringChanged();
    void validChanged();
    void pointCountChanged();
    void cameraCountChanged();
    void maxResectionIdChanged();
    void limitResectionIdChanged();
    void pointCloudBoundingBoxChanged();
    void cameraCenterBoundingBoxChanged();
    void dataReady();

  private slots:
    void onLoadFinished();

  private:
    QString _source;
    bool _loading = false;
    QString _errorString;
    quint32 _limitResectionId = 0;
    std::unique_ptr<SfmDataContent> _content = std::make_unique<SfmDataContent>();
    QFutureWatcher<std::unique_ptr<SfmDataContent>> _watcher;
};
