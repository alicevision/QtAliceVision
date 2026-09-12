#include "CameraInfo.hpp"

CameraInfo::CameraInfo(QObject* parent)
  : QObject(parent)
{}

CameraInfo::~CameraInfo() = default;

void CameraInfo::setBackendProjectionMatrix(const QMatrix4x4& bpm)
{
    _backendProjectionMatrix = bpm;
}

QMatrix4x4 CameraInfo::getProjectionMatrix(double aspectRatio) const
{
    QMatrix4x4 ret = _backendProjectionMatrix;
    ret.perspective(_fov, aspectRatio, _nearPlane, _farPlane);

    return ret;
}

QMatrix4x4 CameraInfo::getOrthogonalMatrix(double aspectRatio) const
{
    Q_UNUSED(aspectRatio);
    return QMatrix4x4();
}
