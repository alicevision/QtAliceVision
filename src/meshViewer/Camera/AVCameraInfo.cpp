#include <Camera/AVCameraInfo.hpp>

AVCameraInfo::AVCameraInfo(QObject* parent)
  : CameraInfo(parent)
{}

bool AVCameraInfo::operator==(const AVCameraInfo& other) const
{
    return _nearPlane == other._nearPlane && _farPlane == other._farPlane && _sx == other._sx && _sy == other._sy && _cx == other._cx &&
           _cy == other._cy && _imageWidth == other._imageWidth && _imageHeight == other._imageHeight && _panX == other._panX &&
           _panY == other._panY && _zoom == other._zoom;
}

QMatrix4x4 AVCameraInfo::getProjectionMatrix(double aspectRatio) const
{
    QMatrix4x4 ret;
    if (_fov > 0)
    {
        ret = _backendProjectionMatrix;
        ret.perspective(_fov, aspectRatio, _nearPlane, _farPlane);
        return ret;
    }

    const double left = (0.0 - _cx) / _sx;
    const double right = (_imageWidth - _cx) / _sx;
    const double top = -(0.0 - _cy) / _sy;
    const double bottom = -(_imageHeight - _cy) / _sy;
    const double sizePlanes = _farPlane - _nearPlane;
    const double horizontalSize = right - left;
    const double verticalSize = top - bottom;
    const double imageAspect = horizontalSize / verticalSize;

    const double scalex = _zoom;
    const double scaley = _zoom * aspectRatio / imageAspect;

    ret(0, 0) = scalex * (2.0 / horizontalSize);
    ret(0, 1) = 0.0;
    ret(0, 2) = scalex * (2.0 * left / horizontalSize + 1.0 - _panX);
    ret(0, 3) = 0.0;

    ret(1, 0) = 0.0;
    ret(1, 1) = scaley * (2.0 / verticalSize);
    ret(1, 2) = scaley * (2.0 * bottom / verticalSize + 1.0 - _panY);
    ret(1, 3) = 0.0;

    ret(2, 0) = 0.0;
    ret(2, 1) = 0.0;
    ret(2, 2) = -(_farPlane + _nearPlane) / sizePlanes;
    ret(2, 3) = -2.0 * _farPlane * _nearPlane / sizePlanes;

    ret(3, 0) = 0.0;
    ret(3, 1) = 0.0;
    ret(3, 2) = -1.0;
    ret(3, 3) = 0.0;

    return _backendProjectionMatrix * ret;
}

QMatrix4x4 AVCameraInfo::getOrthogonalMatrix(double aspectRatio) const
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

    ret(0, 0) = scaleX * _zoom;
    ret(1, 1) = scaleY * _zoom;
    ret(0, 3) = scaleX * _zoom * _panX;
    ret(1, 3) = scaleY * _zoom * _panY;

    return ret;
}
