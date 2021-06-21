#pragma once

#include "DepthMapEntity.hpp"

#include <QtQml>
#include <QQmlExtensionPlugin>

namespace qtAliceVisionImageIO
{

class QtAliceVisionImageIOPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "qtAliceVisionImageIO.qmlPlugin")

public:
    void initializeEngine(QQmlEngine* engine, const char* uri) override {}
    void registerTypes(const char* uri) override
    {
        qInfo() << "[qtAliceVisionImageIO] Plugin Initialized";
        Q_ASSERT(uri == QLatin1String("AliceVisionImageIO"));

        qmlRegisterType<DepthMapEntity>(uri, 1, 0, "DepthMapEntity");
    }
};

} // namespace qtAliceVision
