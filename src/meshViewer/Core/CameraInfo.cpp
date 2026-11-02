#include "CameraInfo.hpp"

CameraInfo::CameraInfo(QObject* parent)
  : QObject(parent)
{}

CameraInfo::~CameraInfo() = default;

void CameraInfo::setBackendProjectionMatrix(const QMatrix4x4& bpm)
{
    _backendProjectionMatrix = bpm;
}
