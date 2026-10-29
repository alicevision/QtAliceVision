#include "OrbitMotionInfo.hpp"

#include <aliceVision/geometry/lie.hpp>
#include <aliceVision/geometry/Pose3.hpp>

namespace {

/**
 * @brief Build the world-to-camera rotation of a camera looking along @p direction, keeping world Y up.
 * The basis is normalized so the rotation stays orthonormal (a scaled basis shears the view once
 * orbit rotations are composed with it). Falls back to world Z as up when looking along Y.
 *
 * @param[in] direction Viewing direction, in world coordinates (need not be normalized).
 * @return The world-to-camera rotation matrix.
 */
Eigen::Matrix3d lookAtRotation(const Eigen::Vector3d& direction)
{
    const Eigen::Vector3d z = -direction.normalized();
    Eigen::Vector3d x = Eigen::Vector3d::UnitY().cross(z);
    if (x.squaredNorm() < 1e-12)
    {
        x = Eigen::Vector3d::UnitZ().cross(z);
    }
    x.normalize();
    const Eigen::Vector3d y = z.cross(x);

    Eigen::Matrix3d R;
    R.col(0) = x;
    R.col(1) = y;
    R.col(2) = z;

    return R.transpose();
}

}  // namespace

OrbitMotionInfo::OrbitMotionInfo(QObject* parent)
  : MotionInfo(parent)
{}

void OrbitMotionInfo::applyTransform()
{
    _center = getCenter();

    Eigen::Matrix3d Rx = aliceVision::SO3::expm({_relativeRotationX * M_PI / 180.0, 0.0, 0.0});
    Eigen::Matrix3d Ry = aliceVision::SO3::expm({0.0, _relativeRotationY * M_PI / 180.0, 0.0});

    Eigen::Matrix4d camera_T_center = _pose.getHomogeneous();
    camera_T_center.topLeftCorner(3, 3) = Rx * _pose.rotation() * Ry;

    _pose = aliceVision::geometry::Pose3(camera_T_center);

    _distance = _distance + _deltaDistance;

    setRelativeRotationX(0.0f);
    setRelativeRotationY(0.0f);
    setPlaneX(0.0f);
    setPlaneY(0.0f);
    setDeltaDistance(0.0f);
    emit changed();
}

void OrbitMotionInfo::setCenter(const QVector3D& center)
{
    aliceVision::geometry::Pose3 cpose(getEigenMatrix());

    _center(0) = center[0];
    _center(1) = center[1];
    _center(2) = center[2];

    Eigen::Vector3d diff = (_center - cpose.center());
    _pose.setRotation(lookAtRotation(diff));
    _distance = diff.norm();

    emit changed();
}

void OrbitMotionInfo::setDistance(float distance)
{
    if (qFuzzyCompare(_distance, distance))
    {
        return;
    }

    _distance = distance;
    
    emit distanceChanged();
    emit changed();
}

Eigen::Matrix4d OrbitMotionInfo::getEigenMatrix() const
{
    Eigen::Matrix4d center_T_world = Eigen::Matrix4d::Identity();
    center_T_world.block<3, 1>(0, 3) = -getCenter();

    Eigen::Matrix3d Rx = aliceVision::SO3::expm({_relativeRotationX * M_PI / 180.0, 0.0, 0.0});
    Eigen::Matrix3d Ry = aliceVision::SO3::expm({0.0, _relativeRotationY * M_PI / 180.0, 0.0});

    Eigen::Matrix4d camera_T_center = _pose.getHomogeneous();
    camera_T_center.topLeftCorner(3, 3) = Rx * _pose.rotation() * Ry;
    camera_T_center(2, 3) -= (_distance + _deltaDistance);

    Eigen::Matrix4d result = camera_T_center * center_T_world;

    return result;
}

QMatrix4x4 OrbitMotionInfo::getMatrix() const
{
    Eigen::Matrix4d result = getEigenMatrix();

    QMatrix4x4 ret;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            ret(i, j) = result(i, j);
        }
    }

    return ret;
}

Eigen::Vector3d OrbitMotionInfo::getCenter() const
{
    Eigen::Vector3d planeMotion;
    planeMotion[0] = _planeX * _distance;
    planeMotion[1] = -_planeY * _distance;
    planeMotion[2] = 0;

    return _pose.rotation().transpose() * planeMotion + _center;
}

void OrbitMotionInfo::fit(const BoundingBox& bbox)
{
    if (!bbox.isValid())   
    {
        return;
    }

    const QVector3D & center = bbox.center();
    _center(0) = center[0];
    _center(1) = center[1];
    _center(2) = center[2];


    Eigen::Vector3d diff = {1.0, -1.0, -1.0};
    _pose.setRotation(lookAtRotation(diff));

    _distance = bbox.maxExtent();

    emit changed();
}

void OrbitMotionInfo::viewTop()
{
    Eigen::Matrix3d R = Eigen::Matrix3d::Zero();

    R(0, 0) = 1;
    R(1, 2) = -1;
    R(2, 1) = 1;

    _pose.setRotation(R);

    emit changed();
}

void OrbitMotionInfo::viewBottom()
{
    Eigen::Matrix3d R = Eigen::Matrix3d::Zero();
    
    R(0, 0) = 1;
    R(1, 2) = 1;
    R(2, 1) = -1;

    _pose.setRotation(R);

    emit changed();
}

void OrbitMotionInfo::viewFront()
{
    Eigen::Matrix3d R = Eigen::Matrix3d::Zero();

    R(0, 0) = 1;
    R(1, 1) = 1;
    R(2, 2) = 1;

    _pose.setRotation(R);

    emit changed();
}

void OrbitMotionInfo::viewBack()
{
    Eigen::Matrix3d R = Eigen::Matrix3d::Zero();
    
    R(0, 0) = -1;
    R(1, 1) = 1;
    R(2, 2) = -1;

    _pose.setRotation(R);

    emit changed();
}

void OrbitMotionInfo::viewLeft()
{
    Eigen::Matrix3d R = Eigen::Matrix3d::Zero();

    R(0, 2) = 1;
    R(1, 1) = 1;
    R(2, 0) = -1;

    _pose.setRotation(R);

    emit changed();
}

void OrbitMotionInfo::viewRight()
{
    Eigen::Matrix3d R = Eigen::Matrix3d::Zero();
    
    R(0, 2) = -1;
    R(1, 1) = 1;
    R(2, 0) = 1;

    _pose.setRotation(R);

    emit changed();
}