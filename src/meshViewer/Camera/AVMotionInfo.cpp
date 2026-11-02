#include "AVMotionInfo.hpp"

AVMotionInfo::AVMotionInfo(QObject* parent)
  : MotionInfo(parent)
{}

QMatrix4x4 AVMotionInfo::getMatrix() const
{
    return _pose;
}