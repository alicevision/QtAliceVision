#pragma once

#include "FeaturesViewer.hpp"
#include <QtQml>
#include <QQmlExtensionPlugin>

#include <aliceVision/system/Logger.hpp>

namespace qtAliceVision
{

class QtAliceVisionPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "qtAliceVision.qmlPlugin")

public:
    void initializeEngine(QQmlEngine* engine, const char* uri) override {}
    void registerTypes(const char* uri) override
    {
        qInfo() << "[QtAliceVision] Plugin Initialized";
        aliceVision::system::Logger::get()->setLogLevel("info");
        Q_ASSERT(uri == QLatin1String("AliceVision"));
        qmlRegisterType<FeaturesViewer>(uri, 1, 0, "FeaturesViewer");
        qmlRegisterUncreatableType<Feature>(uri, 1, 0, "Feature", "Cannot create Feature instances from QML.");
        qRegisterMetaType<QList<Feature*>>( "QList<Feature*>" ); // for usage in signals/slots
    }
};

} // namespace
