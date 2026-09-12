#include "OrbitMotionInfo.hpp"

#include <aliceVision/geometry/lie.hpp>
#include <aliceVision/geometry/Pose3.hpp>

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

void OrbitMotionInfo::setTransform(const QMatrix4x4& transform)
{
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            m(i, j) = transform(i, j);
        }
    }

    _pose = aliceVision::geometry::Pose3(m);
    _center = Eigen::Vector3d::Zero();
    _distance = 0.0f;
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
    Eigen::Vector3d z = -diff.normalized();
    Eigen::Vector3d x = Eigen::Vector3d::UnitY().cross(z);
    Eigen::Vector3d y = z.cross(x);

    Eigen::Matrix3d R;
    R.block<3, 1>(0, 0) = x;
    R.block<3, 1>(0, 1) = y;
    R.block<3, 1>(0, 2) = z;

    _pose.setRotation(R.transpose());
    _distance = diff.norm();

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
