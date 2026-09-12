#include <SfmDataLayer/SfmDataObject.hpp>

#include <aliceVision/sfmData/Landmark.hpp>
#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/sfmDataIO/sfmDataIO.hpp>
#include <aliceVision/camera/Pinhole.hpp>

#include <Camera/AVCameraInfo.hpp>

#include <QtConcurrent/QtConcurrent>
#include <QQmlEngine>
#include <QUrl>

#include <algorithm>
#include <cmath>

namespace {

void updateBoundsForPoint(const QVector3D& point, QVector3D& minPoint, QVector3D& maxPoint, bool& valid)
{
    if (!valid)
    {
        minPoint = point;
        maxPoint = point;
        valid = true;
        return;
    }

    minPoint.setX(std::min(minPoint.x(), point.x()));
    minPoint.setY(std::min(minPoint.y(), point.y()));
    minPoint.setZ(std::min(minPoint.z(), point.z()));

    maxPoint.setX(std::max(maxPoint.x(), point.x()));
    maxPoint.setY(std::max(maxPoint.y(), point.y()));
    maxPoint.setZ(std::max(maxPoint.z(), point.z()));
}

void updateBoundsForCamera(const SfmDataCameraInstance& camera, QVector3D& minPoint, QVector3D& maxPoint, bool& valid)
{
    const QVector3D center = toQMatrix4x4(camera.camera_T_world.inverse()).map(QVector3D());
    updateBoundsForPoint(center, minPoint, maxPoint, valid);
}

std::unique_ptr<SfmDataContent> loadSfmDataFile(const QString& path)
{
    auto result = std::make_unique<SfmDataContent>();

    aliceVision::sfmData::SfMData sfmData;
    if (!aliceVision::sfmDataIO::load(
          sfmData,
          path.toStdString(),
          static_cast<aliceVision::sfmDataIO::ESfMData>(aliceVision::sfmDataIO::VIEWS | aliceVision::sfmDataIO::EXTRINSICS |
                                                        aliceVision::sfmDataIO::INTRINSICS | aliceVision::sfmDataIO::STRUCTURE)))
    {
        result->errorString = QStringLiteral("Failed to load sfmData file: %1").arg(path);
        return result;
    }

    result->points.reserve(sfmData.getLandmarks().size());
    for (const auto& [trackId, landmark] : sfmData.getLandmarks())
    {
        Q_UNUSED(trackId)

        const auto& position = landmark.getX();
        const auto& rgb = landmark.getRgb();
        const QVector3D point(position.x(), position.y(), position.z());

        updateBoundsForPoint(point, result->pointCloudMin, result->pointCloudMax, result->pointCloudBoundsValid);

        result->points.push_back({point, QVector3D(rgb.r() / 255.0f, rgb.g() / 255.0f, rgb.b() / 255.0f)});
    }

    for (const auto [idView, view] : sfmData.getViews().valueRange())
    {
        if (!sfmData.isPoseAndIntrinsicDefined(view))
        {
            continue;
        }

        SfmDataCameraInstance camdata;
        camdata.viewId = idView;
        camdata.frameId = view.getFrameId();
        camdata.path = QString::fromStdString(view.getImage().getImagePath());
        camdata.resectionId = view.getResectionId();
        camdata.camera_T_world = sfmData.getPose(view).getTransform();
        camdata.intrinsics = sfmData.getIntrinsicSharedPtr(view.getIntrinsicId());

        result->cameraPerViewId[camdata.viewId] = camdata;
        updateBoundsForCamera(camdata, result->cameraCenterMin, result->cameraCenterMax, result->cameraCenterBoundsValid);

        if (camdata.resectionId != aliceVision::UndefinedIndexT)
        {
            result->maxResectionId = std::max(result->maxResectionId, camdata.resectionId);
        }

        result->cameras.push_back(camdata);
    }

    result->valid = true;
    return result;
}

}  // namespace

SfmDataObject::SfmDataObject(QObject* parent)
  : QObject(parent)
{
    connect(&_watcher, &QFutureWatcher<std::unique_ptr<SfmDataContent>>::finished, this, &SfmDataObject::onLoadFinished);
}

SfmDataObject::~SfmDataObject()
{
    if (_watcher.isRunning())
    {
        _watcher.waitForFinished();
    }
}

void SfmDataObject::setSource(const QString& path)
{
    if (_source == path)
    {
        return;
    }

    _source = path;
    emit sourceChanged();

    if (path.isEmpty())
    {
        _content = std::make_unique<SfmDataContent>();
        emit validChanged();
        emit pointCountChanged();
        emit cameraCountChanged();
        emit maxResectionIdChanged();
        emit pointCloudBoundingBoxChanged();
        emit cameraCenterBoundingBoxChanged();
        return;
    }

    QString filePath = path;
    if (filePath.startsWith("file://"))
    {
        filePath = QUrl(filePath).toLocalFile();
    }

    _loading = true;
    _errorString.clear();
    emit loadingChanged();
    emit errorStringChanged();

    _watcher.setFuture(QtConcurrent::run([filePath]() { return loadSfmDataFile(filePath); }));
}

bool SfmDataObject::hasCameraTransform(quint32 viewId) const
{
    return _content && _content->cameraPerViewId.find(viewId) != _content->cameraPerViewId.end();
}

QMatrix4x4 SfmDataObject::getCameraTransform(quint32 viewId) const
{
    if (_content)
    {
        const auto it = _content->cameraPerViewId.find(viewId);
        if (it != _content->cameraPerViewId.end())
        {
            return toQMatrix4x4(it->second.camera_T_world);
        }
    }

    return QMatrix4x4();
}

QString SfmDataObject::getImagePath(quint32 viewId) const
{
    if (_content)
    {
        const auto it = _content->cameraPerViewId.find(viewId);
        if (it != _content->cameraPerViewId.end())
        {
            return it->second.path;
        }
    }

    return QString();
}

CameraInfo* SfmDataObject::getCameraInfo(quint32 viewId) const
{
    if (_content)
    {
        const auto it = _content->cameraPerViewId.find(viewId);
        if (it != _content->cameraPerViewId.end())
        {
            auto* info = new AVCameraInfo();
            QQmlEngine::setObjectOwnership(info, QQmlEngine::JavaScriptOwnership);
            const SfmDataCameraInstance& inst = it->second;

            const auto pinhole = std::dynamic_pointer_cast<aliceVision::camera::Pinhole>(inst.intrinsics);
            if (pinhole == nullptr)
            {
                delete info;
                return nullptr;
            }

            info->setNearPlane(0.01f);
            info->setFarPlane(1000.0f);
            info->setSx(pinhole->getScale().x());
            info->setSy(pinhole->getScale().y());
            info->setCx(pinhole->getPrincipalPoint().x());
            info->setCy(pinhole->getPrincipalPoint().y());
            info->setImageWidth(pinhole->w());
            info->setImageHeight(pinhole->h());

            return info;
        }
    }

    return nullptr;
}

void SfmDataObject::setLimitResectionId(quint32 value)
{
    if (_limitResectionId == value)
    {
        return;
    }

    _limitResectionId = value;
    emit limitResectionIdChanged();
}

void SfmDataObject::onLoadFinished()
{
    const bool wasValid = valid();

    _content = _watcher.future().takeResult();
    _loading = false;

    if (!_content->valid)
    {
        _errorString = _content->errorString;
    }
    else
    {
        _errorString.clear();
    }

    _limitResectionId = _content->maxResectionId;

    emit loadingChanged();
    emit errorStringChanged();
    emit pointCountChanged();
    emit cameraCountChanged();
    emit maxResectionIdChanged();
    emit limitResectionIdChanged();
    emit pointCloudBoundingBoxChanged();
    emit cameraCenterBoundingBoxChanged();

    if (valid() != wasValid)
    {
        emit validChanged();
    }

    if (_content->valid)
    {
        emit dataReady();
    }
}

QMatrix4x4 toQMatrix4x4(const aliceVision::geometry::Pose3& pose)
{
    Eigen::Matrix4d M;
    M.setIdentity();
    M(1, 1) = -1;
    M(2, 2) = -1;

    Eigen::Matrix4d buf = M * pose.getHomogeneous() * M;

    QMatrix4x4 ret;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            ret(i, j) = buf(i, j);
        }
    }

    return ret;
}
