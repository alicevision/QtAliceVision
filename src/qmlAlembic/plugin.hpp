#pragma once

#include "AlembicEntity.hpp"
#include <QtQml>
#include <QQmlExtensionPlugin>

namespace abcentity
{

class AlembicEntityQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "alembicEntity.qmlPlugin")

public:
    void initializeEngine(QQmlEngine*, const char*) override {}
    void registerTypes(const char* uri) override
    {
        Q_ASSERT(uri == QLatin1String("AlembicEntity"));
        qmlRegisterType<AlembicEntity>(uri, 2, 0, "AlembicEntity");
        qmlRegisterUncreatableType<CameraLocatorEntity>(uri, 2, 0, "CameraLocatorEntity",
                                                        "Cannot create CameraLocatorEntity instances from QML.");
        qmlRegisterUncreatableType<PointCloudEntity>(uri, 2, 0, "PointCloudEntity",
                                                        "Cannot create PointCloudEntity instances from QML.");
    }
};

} // namespace
