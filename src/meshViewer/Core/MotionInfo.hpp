#pragma once

#include <Eigen/Core>

#include <QObject>
#include <QtQml/qqml.h>
#include <QMatrix4x4>

class MotionInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT

  public:
    explicit MotionInfo(QObject* parent = nullptr);
    ~MotionInfo() override;

    virtual QMatrix4x4 getMatrix() const;

    virtual Eigen::Vector3d getCenter() const;

  signals:
    void changed();
};