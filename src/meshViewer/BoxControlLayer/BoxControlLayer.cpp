#include <BoxControlLayer/BoxControlLayer.hpp>
#include <BoxControlLayer/BoxControlGeometry.hpp>
#include <BoxControlLayer/BoxControlRenderable.hpp>
#include <Core/SceneView.hpp>

#include <algorithm>
#include <limits>

namespace {

/** Hit tolerance around thin handles (shafts, rings), in gizmo units. */
constexpr float pickTolerance = 0.05f;

/**
 * @brief Intersects a ray with a sphere.
 * @return The ray parameter of the first hit in front of the origin, or -1 when missed.
 */
float raySphere(const Ray& ray, const QVector3D& center, float radius)
{
    const QVector3D oc = ray.origin - center;
    const float b = QVector3D::dotProduct(oc, ray.direction);
    const float c = oc.lengthSquared() - radius * radius;
    const float disc = b * b - c;
    if (disc < 0.f)
    {
        return -1.f;
    }

    const float sq = std::sqrt(disc);
    const float t = -b - sq;
    return t > 0.f ? t : (-b + sq > 0.f ? -b + sq : -1.f);
}

/**
 * @brief Tests a ray against a capsule around the segment [p0, p1].
 * @return The ray parameter of the closest approach when within @p radius, or -1.
 */
float rayCapsule(const Ray& ray, const QVector3D& p0, const QVector3D& p1, float radius)
{
    const QVector3D u = ray.direction;
    const QVector3D v = p1 - p0;
    const QVector3D w = ray.origin - p0;
    const float a = QVector3D::dotProduct(u, u);
    const float b = QVector3D::dotProduct(u, v);
    const float c = QVector3D::dotProduct(v, v);
    const float d = QVector3D::dotProduct(u, w);
    const float e = QVector3D::dotProduct(v, w);
    const float denom = a * c - b * b;

    // Closest points between the infinite ray line and the segment
    float s = denom > 1e-8f ? std::clamp((a * e - b * d) / denom, 0.f, 1.f) : 0.f;
    float t = (b * s - d) / a;
    if (t < 0.f)
    {
        t = 0.f;
        s = std::clamp(e / c, 0.f, 1.f);
    }

    const float dist = ((ray.origin + u * t) - (p0 + v * s)).length();
    return dist <= radius ? t : -1.f;
}

/**
 * @brief Tests a ray against a ring of radius @p radius centered at the origin, around @p axis.
 * @return The ray parameter of the hit on the ring plane, or -1.
 */
float rayRing(const Ray& ray, const QVector3D& axis, float radius, float tolerance)
{
    const float denom = QVector3D::dotProduct(ray.direction, axis);
    if (std::abs(denom) < 1e-4f)
    {
        return -1.f;
    }

    const float t = -QVector3D::dotProduct(ray.origin, axis) / denom;
    if (t <= 0.f)
    {
        return -1.f;
    }

    const float dist = (ray.origin + ray.direction * t).length();
    return std::abs(dist - radius) <= tolerance ? t : -1.f;
}

}  // namespace

BoxControlLayer::BoxControlLayer(QObject* parent)
  : LayerItem(parent)
{}

void BoxControlLayer::setTranslation(const QVector3D& translation)
{
    if (_translation == translation)
    {
        return;
    }

    _translation = translation;
    emit translationChanged();
    emit dataReady();
}

void BoxControlLayer::setRotation(const QVector3D& rotation)
{
    if (_rotation == rotation)
    {
        return;
    }

    _rotation = rotation;
    emit rotationChanged();
    emit dataReady();
}

void BoxControlLayer::setScale(const QVector3D& scale)
{
    if (_scale == scale)
    {
        return;
    }

    _scale = scale;
    emit scaleChanged();
    emit dataReady();
}

void BoxControlLayer::setGizmoVisible(bool gizmoVisible)
{
    if (_gizmoVisible == gizmoVisible)
    {
        return;
    }

    _gizmoVisible = gizmoVisible;
    emit gizmoVisibleChanged();
    emit dataReady();
}

void BoxControlLayer::setGizmoSize(float gizmoSize)
{
    if (qFuzzyCompare(_gizmoSize, gizmoSize))
    {
        return;
    }

    _gizmoSize = gizmoSize;
    emit gizmoSizeChanged();
    emit dataReady();
}

void BoxControlLayer::setHighlightedHandle(int handle)
{
    if (_highlightedHandle == handle)
    {
        return;
    }

    _highlightedHandle = handle;
    emit highlightedHandleChanged();
    emit dataReady();
}

QMatrix4x4 BoxControlLayer::boxMatrix() const
{
    QMatrix4x4 m;
    m.translate(_translation);
    m.rotate(orientation());
    m.scale(_scale);
    return m;
}

float BoxControlLayer::gizmoScaleFor(const QMatrix4x4& viewProjection, float projectionScaleY, const QVector3D& center, float gizmoSize)
{
    // Scaling by clip-space w keeps a constant screen size (w = depth in perspective, 1 in orthographic).
    // Dividing by the projection vertical scale makes the size relative to the viewport height.
    // The 2.5 factor gives a size close to the former Qt3D gizmo for the default gizmoSize.
    const float w = (viewProjection * QVector4D(center, 1.f)).w();
    const float projScale = std::max(std::abs(projectionScaleY), 1e-6f);
    return gizmoSize * 2.5f * std::abs(w) / projScale;
}

QVector3D BoxControlLayer::facePosition(int axis, float sign) const
{
    return _translation + axisDirection(axis) * (sign * (_scale[axis] + boxControl::faceOffset));
}

QVector3D BoxControlLayer::axisDirection(int axis) const
{
    if (axis < 0 || axis > 2)
    {
        return {};
    }

    return orientation().rotatedVector(boxControl::axisVector(axis));
}

float BoxControlLayer::gizmoWorldScale(SceneView* view) const
{
    if (!view)
    {
        return 1.f;
    }

    const QMatrix4x4 proj = view->projectionMatrix();
    return gizmoScaleFor(proj * view->viewMatrix(), proj(1, 1), _translation, _gizmoSize);
}

int BoxControlLayer::handleAxis(int handle)
{
    if (handle >= TranslateX && handle <= ScaleZ)
    {
        return (handle - TranslateX) % 3;
    }
    if (handle >= FaceXPos && handle <= FaceZNeg)
    {
        return (handle - FaceXPos) / 2;
    }
    return -1;
}

int BoxControlLayer::handleSign(int handle)
{
    if (handle >= FaceXPos && handle <= FaceZNeg)
    {
        return ((handle - FaceXPos) % 2 == 0) ? 1 : -1;
    }
    return 0;
}

int BoxControlLayer::handleAt(SceneView* view, const QVector2D& mousePos) const
{
    if (!view || !_visible || !_gizmoVisible)
    {
        return NoHandle;
    }

    const Ray worldRay = view->screenRay(mousePos);
    if (worldRay.direction.isNull())
    {
        return NoHandle;
    }

    // Express the ray in gizmo space: origin at the box center, box axes, gizmo units.
    // Ray parameters then scale uniformly with world ones, so they can be compared directly.
    const float g = gizmoWorldScale(view);
    const QQuaternion invRot = orientation().inverted();
    const Ray ray{invRot.rotatedVector(worldRay.origin - _translation) / g, invRot.rotatedVector(worldRay.direction)};

    int bestHandle = NoHandle;
    float bestT = std::numeric_limits<float>::max();
    auto consider = [&](float t, int handle) {
        if (t > 0.f && t < bestT)
        {
            bestT = t;
            bestHandle = handle;
        }
    };

    using namespace boxControl;
    for (int axis = 0; axis < 3; ++axis)
    {
        const QVector3D dir = axisVector(axis);

        const float coneStart = translateHandleDistance(_scale[axis]) / g;
        consider(rayCapsule(ray, dir * coneStart, dir * (coneStart + coneLength), std::max(coneRadius, pickTolerance)), TranslateX + axis);
        consider(raySphere(ray, dir * shaftEnd, std::max(scaleCubeEdge, pickTolerance)), ScaleX + axis);
        consider(rayRing(ray, dir, torusRadiusForAxis(axis), pickTolerance * 0.6f), RotateX + axis);

        // Face handles are placed in world units from the box faces
        for (int s = 0; s < 2; ++s)
        {
            const float sign = s == 0 ? 1.f : -1.f;
            const QVector3D center = dir * (sign * (_scale[axis] + faceOffset) / g);
            consider(raySphere(ray, center, std::max(faceRadius, pickTolerance)), FaceXPos + 2 * axis + s);
        }
    }

    return bestHandle;
}

BoundingBox BoxControlLayer::boundingBox() const
{
    const QMatrix4x4 m = boxMatrix();
    BoundingBox box;
    for (int i = 0; i < 8; ++i)
    {
        box.extend(m.map(QVector3D((i & 1) ? 1.f : -1.f, (i & 2) ? 1.f : -1.f, (i & 4) ? 1.f : -1.f)));
    }
    return box;
}

std::unique_ptr<IRenderable> BoxControlLayer::createRenderable() const
{
    return std::make_unique<BoxControlRenderable>();
}
