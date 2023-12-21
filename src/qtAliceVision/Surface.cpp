#include "Surface.hpp"

#include <aliceVision/camera/Equidistant.hpp>
#include <aliceVision/camera/IntrinsicBase.hpp>
#include <aliceVision/camera/Pinhole.hpp>

#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/sfmDataIO/sfmDataIO.hpp>

// Import M_PI
#include <math.h>
#include <memory>

namespace qtAliceVision {

aliceVision::Vec2 toEquirectangular(const aliceVision::Vec3& spherical, int width, int height)
{
    const double vertical_angle = asin(spherical(1));
    const double horizontal_angle = atan2(spherical(0), spherical(2));

    const double latitude = ((vertical_angle + M_PI_2) / M_PI) * height;
    const double longitude = ((horizontal_angle + M_PI) / (2.0 * M_PI)) * width;

    return aliceVision::Vec2(longitude, latitude);
}

Surface::Surface(int subdivisions, QObject* parent)
  : QObject(parent)
{
    updateSubdivisions(subdivisions);
    connect(this, &Surface::sfmDataChanged, this, &Surface::msfmDataUpdate);
    connect(this, &Surface::anglesChanged, this, &Surface::verticesChanged);
}

Surface::~Surface() {}

void Surface::update(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize, int downscaleLevel)
{
    // Compute Vertices coordinates and Indices order
    computeGrid(vertices, indices, textureSize, downscaleLevel);

    // If Panorama has been rotated, reset values and return true
    if (_isPanoramaRotating)
        _isPanoramaRotating = false;
}

// GRID METHODS
void Surface::computeGrid(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize, int downscaleLevel)
{
    bool verticesComputed = false;
    if (_sfmLoaded && (_isPanoramaRotating || _needToUseIntrinsic))
    {
        // Load Intrinsic with 2 ways whether we are in the Panorama or Distorsion Viewer
        const aliceVision::camera::IntrinsicBase* intrinsic = (_msfmData)?getIntrinsicFromViewId(_idView):nullptr;

        if (intrinsic)
        {
            computeVerticesGrid(vertices, textureSize, intrinsic, downscaleLevel);
            verticesComputed = true;
            setVerticesChanged(true);
            Q_EMIT verticesChanged();
            _needToUseIntrinsic = false;
        }
        else
        {
            verticesComputed = false;
        }
    }
    // If there is no sfm data update or intrinsics are invalid, keep the same vertices
    if (!verticesComputed)
    {
        computeVerticesGrid(vertices, textureSize, nullptr);
        setVerticesChanged(false);
    }

    computeIndicesGrid(indices);
}

void Surface::computeGrid(QSGGeometry* geometryLine)
{
    removeGrid(geometryLine);

    int countPoint = 0;
    int index = 0;
    for (int i = 0; i <= _subdivisions; i++)
    {
        for (int j = 0; j <= _subdivisions; j++)
        {
            if (i == _subdivisions && j == _subdivisions)
                continue;

            // Horizontal Lines
            if (i != _subdivisions)
            {
                geometryLine->vertexDataAsPoint2D()[countPoint++].set(static_cast<float>(_vertices[index].x()),
                                                                      static_cast<float>(_vertices[index].y()));
                index += _subdivisions + 1;
                geometryLine->vertexDataAsPoint2D()[countPoint++].set(static_cast<float>(_vertices[index].x()),
                                                                      static_cast<float>(_vertices[index].y()));
                index -= _subdivisions + 1;

                if (j == _subdivisions)
                {
                    index++;
                    continue;
                }
            }

            // Vertical Lines
            geometryLine->vertexDataAsPoint2D()[countPoint++].set(static_cast<float>(_vertices[index].x()), static_cast<float>(_vertices[index].y()));
            index++;
            geometryLine->vertexDataAsPoint2D()[countPoint++].set(static_cast<float>(_vertices[index].x()), static_cast<float>(_vertices[index].y()));
        }
    }
}

void Surface::computeVerticesGrid(QSGGeometry::TexturedPoint2D* vertices,
                                  QSize textureSize,
                                  const aliceVision::camera::IntrinsicBase* intrinsic,
                                  int downscaleLevel)
{
    // Retrieve pose if Panorama Viewer is enable
    if (isPanoramaViewerEnabled() && intrinsic)
    {
        // Downscale image according to downscale level
        textureSize *= pow(2.0, downscaleLevel);
        resetValuesVertexEnabled();
    }

    // Retrieve pose
    aliceVision::sfmData::CameraPose pose;
    if (isPanoramaViewerEnabled() && intrinsic && _msfmData)
    {
        const auto viewIt = _msfmData->rawData().getViews().find(_idView);
        const aliceVision::sfmData::View& view = *viewIt->second;
        pose = _msfmData->rawData().getPose(view);
    }

    const aliceVision::camera::Equidistant* eqcam = dynamic_cast<const aliceVision::camera::Equidistant*>(intrinsic);
    aliceVision::Vec2 center = {0, 0};
    double radius = std::numeric_limits<double>::max();

    if (eqcam)
    {
        center = {eqcam->getCircleCenterX(), eqcam->getCircleCenterY()};
        radius = eqcam->getCircleRadius();
    }

    bool fillCoordsSphere = _defaultSphereCoordinates.empty();
    int vertexIndex = 0;
    float fSubdivisions = static_cast<float>(_subdivisions);
    for (size_t i = 0; i <= static_cast<size_t>(_subdivisions); i++)
    {
        for (size_t j = 0; j <= static_cast<size_t>(_subdivisions); j++)
        {
            float fI = static_cast<float>(i);
            float fJ = static_cast<float>(j);
            float x, y = 0.f;

            /* The panorama viewer needs to use the previously computed vertices to compute the current ones,
             * otherwise images will end up stacked together on the top-left corner.
             * For anything other than the panorama viewer, the surface vertices should be recomputed from scratch
             * so the distorsions are not accumulated. */
            if (_vertices.empty() || !isPanoramaViewerEnabled())
            {
                x = fI * static_cast<float>(textureSize.width()) / fSubdivisions;
                y = fJ * static_cast<float>(textureSize.height()) / fSubdivisions;
            }
            else
            {
                x = static_cast<float>(_vertices[vertexIndex].x());
                y = static_cast<float>(_vertices[vertexIndex].y());
            }

            const double cx = x - center(0);
            const double cy = y - center(1);
            const double dist = std::hypot(cx, cy);
            const double maxradius = 0.99 * radius;

            if (dist > maxradius)
            {
                x = static_cast<float>(center(0) + maxradius * cx / dist);
                y = static_cast<float>(center(1) + maxradius * cy / dist);
            }

            const float u = fI / fSubdivisions;
            const float v = fJ / fSubdivisions;

            // Remove Distortion only if sfmData has been updated
            if (isDistortionViewerEnabled() && intrinsic && intrinsic->hasDistortion())
            {
                const aliceVision::Vec2 undisto_pix(x, y);
                const aliceVision::Vec2 disto_pix = intrinsic->get_d_pixel(undisto_pix);
                vertices[vertexIndex].set(static_cast<float>(disto_pix.x()), static_cast<float>(disto_pix.y()), u, v);
            }

            // Equirectangular Convertion only if sfmData has been updated
            if (isPanoramaViewerEnabled() && intrinsic)
            {
                // Compute pixel coordinates on the Unit Sphere
                if (fillCoordsSphere)
                {
                    // Image System Coordinates
                    aliceVision::Vec2 uvCoord(x, y);
                    const auto& transfromPose = pose.getTransform();
                    _defaultSphereCoordinates.push_back(aliceVision::camera::applyIntrinsicExtrinsic(transfromPose, intrinsic, uvCoord));
                }

                // Rotate Panorama if some rotation values exist
                aliceVision::Vec3 sphereCoordinates(_defaultSphereCoordinates[static_cast<size_t>(vertexIndex)]);
                rotatePanorama(sphereCoordinates);

                // Compute pixel coordinates in the panorama coordinate system
                aliceVision::Vec2 panoramaCoordinates = toEquirectangular(sphereCoordinates, _panoramaWidth, _panoramaHeight);

                // If image is on the seam
                if (vertexIndex > 0 && j > 0)
                {
                    double deltaX = panoramaCoordinates.x() - vertices[vertexIndex - 1].x;
                    if (abs(deltaX) > 0.7 * _panoramaWidth)
                    {
                        _vertexEnabled[i][j - 1] = false;
                    }
                }
                if (vertexIndex >= (_subdivisions + 1))
                {
                    double deltaY = panoramaCoordinates.x() - vertices[vertexIndex - (_subdivisions + 1)].x;
                    if (abs(deltaY) > 0.7 * _panoramaWidth && j > 0)
                    {
                        _vertexEnabled[i][j - 1] = false;
                    }
                }
                vertices[vertexIndex].set(static_cast<float>(panoramaCoordinates.x()), static_cast<float>(panoramaCoordinates.y()), u, v);
            }

            // Default
            if (!intrinsic)
            {
                vertices[vertexIndex].set(x, y, u, v);
            }
            vertexIndex++;
        }
    }
    // End for loop
    Q_EMIT verticesChanged();
}

bool Surface::isPointValid(size_t i, size_t j) const
{
    // Check if the point and all its neighbours are enabled
    // If not, return false
    if (!_vertexEnabled[i][j])
        return false;

    if (i > 0)
    {
        if (!_vertexEnabled[i - 1][j])
            return false;
    }

    if (j > 0)
    {
        if (!_vertexEnabled[i][j - 1])
            return false;
    }

    if (i > 0 && j > 0)
    {
        if (!_vertexEnabled[i - 1][j - 1])
            return false;
    }

    if (static_cast<int>(i) < _subdivisions + 1)
    {
        if (!_vertexEnabled[i + 1][j])
            return false;
        if (j > 0 && !_vertexEnabled[i + 1][j - 1])
            return false;
    }

    if (static_cast<int>(j) < _subdivisions + 1)
    {
        if (!_vertexEnabled[i][j + 1])
            return false;
        if (i > 0 && !_vertexEnabled[i - 1][j + 1])
            return false;
    }

    if (static_cast<int>(i) < _subdivisions + 1 && static_cast<int>(j) < _subdivisions + 1)
    {
        if (!_vertexEnabled[i + 1][j + 1])
            return false;
    }

    return true;
}

void Surface::resetValuesVertexEnabled()
{
    for (size_t i = 0; i <= static_cast<size_t>(_subdivisions); i++)
    {
        for (size_t j = 0; j <= static_cast<size_t>(_subdivisions); j++)
        {
            _vertexEnabled[j][i] = true;
        }
    }
}

void Surface::computeIndicesGrid(quint16* indices)
{
    int index = 0;
    for (size_t j = 0; j < static_cast<size_t>(_subdivisions); j++)
    {
        for (size_t i = 0; i < static_cast<size_t>(_subdivisions); i++)
        {
            if (!isPanoramaViewerEnabled() || (isPanoramaViewerEnabled() && isPointValid(i, j)))
            {
                int topLeft = (static_cast<int>(i) * (_subdivisions + 1)) + static_cast<int>(j);
                int topRight = topLeft + 1;
                int bottomLeft = topLeft + _subdivisions + 1;
                int bottomRight = bottomLeft + 1;
                indices[index++] = static_cast<quint16>(topLeft);
                indices[index++] = static_cast<quint16>(bottomLeft);
                indices[index++] = static_cast<quint16>(topRight);
                indices[index++] = static_cast<quint16>(topRight);
                indices[index++] = static_cast<quint16>(bottomLeft);
                indices[index++] = static_cast<quint16>(bottomRight);
            }
            else
            {
                indices[index++] = 0;
                indices[index++] = 0;
                indices[index++] = 0;
                indices[index++] = 0;
                indices[index++] = 0;
                indices[index++] = 0;
            }
        }
    }
    _indices.clear();
    for (size_t i = 0; i < static_cast<size_t>(_indexCount); i++)
        _indices.append(indices[i]);
}

void Surface::removeGrid(QSGGeometry* geometryLine)
{
    for (size_t i = 0; i < static_cast<size_t>(geometryLine->vertexCount()); i++)
    {
        geometryLine->vertexDataAsPoint2D()[i].set(0, 0);
    }
}

void Surface::setGridColor(const QColor& color)
{
    if (_gridColor == color)
        return;

    _gridColor = color;
    _gridColor.setAlpha(_gridOpacity);
    Q_EMIT gridColorChanged(color);
}
void Surface::setGridOpacity(const int& opacity)
{
    if (_gridOpacity == opacity)
        return;

    if (_gridOpacity == int((opacity / 100.0) * 255))
        return;
    _gridOpacity = int((opacity / 100.0) * 255);
    _gridColor.setAlpha(_gridOpacity);
    Q_EMIT gridOpacityChanged(opacity);
}

void Surface::setDisplayGrid(bool display)
{
    if (_displayGrid == display)
        return;

    _displayGrid = display;
    Q_EMIT displayGridChanged();
}

QPointF Surface::getPrincipalPoint()
{
    aliceVision::Vec2 ppCorrection(0.0, 0.0);

    const aliceVision::camera::IntrinsicBase* intrinsic = (_msfmData)? getIntrinsicFromViewId(_idView): nullptr;

    if (intrinsic && aliceVision::camera::isPinhole(intrinsic->getType()))
    {
        ppCorrection = dynamic_cast<const aliceVision::camera::IntrinsicScaleOffset&>(*intrinsic).getOffset();
    }

    return {ppCorrection.x(), ppCorrection.y()};
}

// VERTICES FUNCTION
void Surface::fillVertices(QSGGeometry::TexturedPoint2D* vertices)
{
    _vertices.clear();
    for (int i = 0; i < _vertexCount; i++)
    {
        QPoint p(static_cast<int>(vertices[i].x), static_cast<int>(vertices[i].y));
        _vertices.append(p);
    }
}

// SUBDIVISIONS FUNCTIONS
void Surface::updateSubdivisions(int sub)
{
    _subdivisions = sub;

    // Update vertexCount and indexCount according to new subdivision count
    _vertexCount = (_subdivisions + 1) * (_subdivisions + 1);
    _indexCount = _subdivisions * _subdivisions * 6;

    _vertexEnabled.resize(static_cast<size_t>(_subdivisions) + 1);
    for (size_t i = 0; i < static_cast<size_t>(_subdivisions) + 1; i++)
        _vertexEnabled[i].resize(static_cast<size_t>(_subdivisions) + 1);
}

void Surface::setSubdivisions(int newSubdivisions)
{
    if (newSubdivisions == _subdivisions)
        return;

    setHasSubdivisionsChanged(true);
    updateSubdivisions(newSubdivisions);

    clearVertices();
    setVerticesChanged(true);
    _needToUseIntrinsic = true;
    Q_EMIT subdivisionsChanged();
}

// PANORAMA
void Surface::rotatePanorama(aliceVision::Vec3& coordSphere)
{
    Eigen::AngleAxis<double> Myaw(_yaw, Eigen::Vector3d::UnitY());
    Eigen::AngleAxis<double> Mpitch(_pitch, Eigen::Vector3d::UnitX());
    Eigen::AngleAxis<double> Mroll(_roll, Eigen::Vector3d::UnitZ());

    Eigen::Matrix3d cRo = Myaw.toRotationMatrix() * Mpitch.toRotationMatrix() * Mroll.toRotationMatrix();

    coordSphere = cRo * coordSphere;
}

double Surface::getPitch()
{
    // Get pitch in degrees
    return getEulerAngleDegrees(_pitch);
}

void Surface::setPitch(double pitchInDegrees)
{
    _pitch = aliceVision::degreeToRadian(pitchInDegrees);
    _isPanoramaRotating = true;
    setVerticesChanged(true);

    Q_EMIT anglesChanged();
}

double Surface::getYaw()
{
    // Get yaw in degrees
    return getEulerAngleDegrees(_yaw);
}

void Surface::setYaw(double yawInDegrees)
{
    _yaw = aliceVision::degreeToRadian(yawInDegrees);
    _isPanoramaRotating = true;
    setVerticesChanged(true);

    Q_EMIT anglesChanged();
}

double Surface::getRoll()
{
    // Get roll in degrees
    return getEulerAngleDegrees(_roll);
}
void Surface::setRoll(double rollInDegrees)
{
    _roll = aliceVision::degreeToRadian(rollInDegrees);
    _isPanoramaRotating = true;
    setVerticesChanged(true);

    Q_EMIT anglesChanged();
}

double Surface::getEulerAngleDegrees(double angleRadians)
{
    double angleDegrees = angleRadians;
    int power = static_cast<int>(angleDegrees / M_PI);
    angleDegrees = fmod(angleDegrees, M_PI) * pow(-1, power);

    // Radians to Degrees
    angleDegrees *= (180.0f / M_PI);
    if (power % 2 != 0)
        angleDegrees = -180.0 - angleDegrees;

    return angleDegrees;
}

// ID VIEW FUNCTION
void Surface::setIdView(int id)
{
    // Handles cases where the sent view ID is unknown or unset (equals  to -1)
    if (id >= 0)
        _idView = static_cast<uint>(id);
    else
        _idView = 0;
}

// MOUSE FUNCTIONS
bool Surface::isMouseInside(float mx, float my)
{
    QPointF P(mx, my);
    bool inside = false;

    for (int i = 0; i < indexCount(); i += 3)
    {
        if (i + 2 >= _indices.size())
            break;

        QPointF A = _vertices[_indices[i]];
        QPointF B = _vertices[_indices[i + 1]];
        QPointF C = _vertices[_indices[i + 2]];

        // Compute vectors
        QPointF v0 = C - A;
        QPointF v1 = B - A;
        QPointF v2 = P - A;

        // Compute dot products
        double dot00 = QPointF::dotProduct(v0, v0);
        double dot01 = QPointF::dotProduct(v0, v1);
        double dot02 = QPointF::dotProduct(v0, v2);
        double dot11 = QPointF::dotProduct(v1, v1);
        double dot12 = QPointF::dotProduct(v1, v2);

        // Compute barycentric coordinates
        const double dots = (dot00 * dot11 - dot01 * dot01);
        if (dots == 0)
            continue;

        double invDenom = 1 / dots;
        double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
        double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

        // Check if point is in triangle
        if ((u >= 0) && (v >= 0) && (u + v < 1))
        {
            inside = true;
            break;
        }
    }
    return inside;
}

void Surface::setMouseOver(bool state)
{
    if (state == _mouseOver)
        return;

    _mouseOver = state;
    Q_EMIT mouseOverChanged();
}

// VIEWER TYPE FUNCTIONS

void Surface::setViewerType(EViewerType type)
{
    if (_viewerType == type)
        return;

    _viewerType = type;
    clearVertices();
    setVerticesChanged(true);
    Q_EMIT viewerTypeChanged();
}

bool Surface::isPanoramaViewerEnabled() const { return _viewerType == EViewerType::PANORAMA; }

bool Surface::isDistortionViewerEnabled() const { return _viewerType == EViewerType::DISTORTION; }
bool Surface::isHDRViewerEnabled() const { return _viewerType == EViewerType::HDR; }

void Surface::setMSfmData(MSfMData* sfmData)
{
    _sfmLoaded = false;

    if (_msfmData == sfmData)
    {
        _sfmLoaded = true;
        return;
    }

    if (_msfmData != nullptr)
    {
        disconnect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    _msfmData = sfmData;

    if (!_msfmData)
        return;

    if (_msfmData != nullptr)
    {
        connect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }

    if (_msfmData->status() != MSfMData::Ready)
    {
        qWarning() << "[QtAliceVision] SURFACE setMSfmData: SfMData is not ready: " << _msfmData->status();
        return;
    }
    if (_msfmData->rawData().getViews().empty())
    {
        qWarning() << "[QtAliceVision] SURFACE setMSfmData: SfMData is empty";
        return;
    }

    Q_EMIT sfmDataChanged();
}

const aliceVision::camera::IntrinsicBase* Surface::getIntrinsicFromViewId(unsigned int viewId) const
{
    if (!_msfmData)
    {
        return nullptr;
    }
    
    const auto viewIt = _msfmData->rawData().getViews().find(viewId);
    if (viewIt == _msfmData->rawData().getViews().end())
    {
        return nullptr;
    }
    const aliceVision::sfmData::View& view = *viewIt->second;
    return _msfmData->rawData().getIntrinsicPtr(view.getIntrinsicId());
}

const aliceVision::camera::Equidistant* Surface::getIntrinsicEquidistant() const
{
    const aliceVision::camera::IntrinsicBase* intrinsic = getIntrinsicFromViewId(_idView);
    if (!intrinsic)
    {
        return nullptr;
    }

    // Load equidistant intrinsic (the intrinsic for full circle fisheye cameras)
    const aliceVision::camera::Equidistant* intrinsicEquidistant = dynamic_cast<const aliceVision::camera::Equidistant*>(intrinsic);

    return intrinsicEquidistant;
}

}  // namespace qtAliceVision