#include <Camera/BaseCameraInfo.hpp>

BaseCameraInfo::BaseCameraInfo(QObject* parent)
  : CameraInfo(parent)
{}

bool BaseCameraInfo::operator==(const BaseCameraInfo& other) const
{
    return _fov == other._fov && _farPlane == other._farPlane && _imageWidth == other._imageWidth && _imageHeight == other._imageHeight &&
           _orthographic == other._orthographic && _orthographicWidth == other._orthographicWidth;
}

QMatrix4x4 BaseCameraInfo::getProjectionMatrix(double aspectRatio) const
{
    if (aspectRatio <= 0.0)
    {
        aspectRatio = 1.0;
    }

    QMatrix4x4 ret = _backendProjectionMatrix;

    if (_orthographic)
    {
        const float halfWidth = 0.5f * _orthographicWidth;
        const float halfHeight = halfWidth / static_cast<float>(aspectRatio);
        ret.ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, _nearPlane, _farPlane);
    }
    else
    {
        ret.perspective(_fov, aspectRatio, _nearPlane, _farPlane);
    }

    return ret;
}

QMatrix4x4 BaseCameraInfo::getOrthogonalMatrix(double aspectRatio) const
{
    QMatrix4x4 ret;

    if (_imageWidth <= 0 || _imageHeight <= 0 || aspectRatio <= 0.0)
    {
        return ret;
    }

    const double imageAspect = static_cast<double>(_imageWidth) / static_cast<double>(_imageHeight);

    double scaleX = 1.0;
    double scaleY = 1.0;
    if (imageAspect > aspectRatio)
    {
        scaleX = 1.0;
        scaleY = aspectRatio / imageAspect;
    }
    else
    {
        scaleX = imageAspect / aspectRatio;
        scaleY = 1.0;
    }

    ret(0, 0) = scaleX;
    ret(1, 1) = scaleY;

    return ret;
}
