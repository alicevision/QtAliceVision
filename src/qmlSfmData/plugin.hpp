#pragma once

#include <QtQml>
#include <QQmlExtensionPlugin>

namespace sfmdataentity
{

class sfmDataEntityQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "sfmDataEntity.qmlPlugin")

public:
    void initializeEngine(QQmlEngine*, const char*) override {}
    void registerTypes(const char* uri) override
    {
        Q_ASSERT(uri == QLatin1String("SfmDataEntity"));
        
        qmlRegisterType<SfmDataEntity>(uri, 1, 0, "SfmDataEntity");
        qmlRegisterUncreatableType<CameraLocatorEntity>(uri, 1, 0, "CameraLocatorEntity", "Cannot create CameraLocatorEntity instances from QML.");
        qmlRegisterUncreatableType<PointCloudEntity>(uri, 1, 0, "PointCloudEntity", "Cannot create PointCloudEntity instances from QML.");
    }
};

} // namespace
