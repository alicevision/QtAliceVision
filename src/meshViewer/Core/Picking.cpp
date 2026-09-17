#include <Core/Picking.hpp>

Ray unprojectRay(const QMatrix4x4& projection, const QMatrix4x4& transform, const QVector2D& mousePos, float viewportWidth, float viewportHeight)
{
    // Screen → NDC.  Mouse origin is top-left, NDC Y points up.
    const float ndcX = 2.0f * mousePos.x() / viewportWidth - 1.0f;
    const float ndcY = -2.0f * mousePos.y() / viewportHeight + 1.0f;

    bool invertible = false;
    const QMatrix4x4 invPV = (projection * transform).inverted(&invertible);
    if (!invertible)
        return {};

    // Unproject two NDC depths to world space and build a ray between them.
    auto unproject = [&](float ndcZ) -> QVector3D {
        const QVector4D world = invPV * QVector4D(ndcX, ndcY, ndcZ, 1.0f);
        return QVector3D(world) / world.w();
    };

    const QVector3D nearPt = unproject(-1.0f);  // near clip plane
    const QVector3D farPt = unproject(1.0f);    // far clip plane

    return {nearPt, (farPt - nearPt).normalized()};
}