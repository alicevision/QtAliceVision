#include <Camera/AVCameraInfo.hpp>

namespace {

/**
 * @brief Compute the NDC scale fitting an image inside a viewport while preserving its aspect ratio.
 *
 * The image is letterboxed (or pillarboxed) so that it covers the largest possible area of the viewport
 * without distortion. Using the same scale for the projected scene and the image guarantees that both
 * share the same footprint on screen.
 *
 * @param[in] imageAspect Aspect ratio (width / height) of the image.
 * @param[in] aspectRatio Aspect ratio (width / height) of the viewport.
 * @param[out] scaleX Horizontal scale in NDC, in ]0, 1].
 * @param[out] scaleY Vertical scale in NDC, in ]0, 1].
 */
void fitImageScale(double imageAspect, double aspectRatio, double& scaleX, double& scaleY)
{
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
}

}  // namespace

/**
 * @brief Construct a camera with default intrinsics (unit focal, principal point at origin, empty image).
 * @param[in] parent Optional Qt parent object.
 */
AVCameraInfo::AVCameraInfo(QObject* parent)
  : CameraInfo(parent)
{}

/**
 * @brief Compare two cameras on every parameter that affects their matrices.
 * @param[in] other Camera to compare with.
 * @return True if clipping planes, intrinsics, image size, pan and zoom are all exactly equal.
 * @note Comparison is exact; the fuzzy comparison used by the setters is not applied here.
 */
bool AVCameraInfo::operator==(const AVCameraInfo& other) const
{
    return _nearPlane == other._nearPlane && _farPlane == other._farPlane && _sx == other._sx && _sy == other._sy && _cx == other._cx &&
           _cy == other._cy && _imageWidth == other._imageWidth && _imageHeight == other._imageHeight && _panX == other._panX &&
           _panY == other._panY && _zoom == other._zoom;
}

/**
 * @brief Build the perspective projection matrix matching the pinhole intrinsics.
 *
 * The image bounds are expressed on the normalized image plane (z = 1) using the focal lengths
 * (@c sx, @c sy) and principal point (@c cx, @c cy), then mapped to an off-axis OpenGL-style frustum.
 * The image Y axis points down while the camera Y axis points up, hence the sign flip on @c top / @c bottom.
 *
 * The frustum is then scaled so the image fits the viewport without distortion (see fitImageScale()),
 * and the user @c zoom and @c pan are applied in NDC.
 *
 * @param[in] aspectRatio Aspect ratio (width / height) of the viewport.
 * @return The projection matrix, premultiplied by the backend-specific projection correction.
 * @note If the image size or @p aspectRatio is not strictly positive, no letterboxing is applied.
 */
QMatrix4x4 AVCameraInfo::getProjectionMatrix(double aspectRatio) const
{
    QMatrix4x4 ret;

    const double left = (0.0 - _cx) / _sx;
    const double right = (_imageWidth - _cx) / _sx;
    const double top = -(0.0 - _cy) / _sy;
    const double bottom = -(_imageHeight - _cy) / _sy;
    const double sizePlanes = _farPlane - _nearPlane;
    const double horizontalSize = right - left;
    const double verticalSize = top - bottom;

    double scalex = 1.0;
    double scaley = 1.0;
    if (_imageWidth > 0 && _imageHeight > 0 && aspectRatio > 0.0)
    {
        fitImageScale(static_cast<double>(_imageWidth) / static_cast<double>(_imageHeight), aspectRatio, scalex, scaley);
    }
    scalex *= _zoom;
    scaley *= _zoom;

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

/**
 * @brief Build the 2D NDC transform used to display the camera image in the viewport.
 *
 * The image quad spanning [-1, 1] is scaled to fit the viewport without distortion (see fitImageScale()),
 * then zoomed and panned consistently with getProjectionMatrix() so the image stays aligned with the scene.
 *
 * @param[in] aspectRatio Aspect ratio (width / height) of the viewport.
 * @return The scale and translation matrix, or identity if the image size or @p aspectRatio is not strictly positive.
 */
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
    fitImageScale(imageAspect, aspectRatio, scaleX, scaleY);

    ret(0, 0) = scaleX * _zoom;
    ret(1, 1) = scaleY * _zoom;
    ret(0, 3) = scaleX * _zoom * _panX;
    ret(1, 3) = scaleY * _zoom * _panY;

    return ret;
}
