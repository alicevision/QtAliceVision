#include "MotionInfo.hpp"

MotionInfo::MotionInfo(QObject* parent)
  : QObject(parent)
{}

MotionInfo::~MotionInfo() = default;

QMatrix4x4 MotionInfo::getMatrix() const
{
    return QMatrix4x4();
}

Eigen::Vector3d MotionInfo::getCenter() const
{
    return Eigen::Vector3d::Zero();
}