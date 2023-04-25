#pragma once

#include "ImageServer.hpp"
#include "FeaturesViewer.hpp"
#include "FloatImageViewer.hpp"
#include "MFeatures.hpp"
#include "MSfMDataStats.hpp"
#include "MTracks.hpp"
#include "MViewStats.hpp"
#include "PanoramaViewer.hpp"
#include "Surface.hpp"
#include "MFeatures.hpp"

#include <aliceVision/system/Logger.hpp>

#include <QtQml>
#include <QQmlExtensionPlugin>

#include <memory>

namespace qtAliceVision
{

class QtAliceVisionPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "qtAliceVision.qmlPlugin")

public:
    void initializeEngine(QQmlEngine* engine, const char* uri) override
    {
        // Fix "unused parameter" warnings; should be replaced by [[maybe_unused]] when C++17 is supported
        (void)engine;
        (void)uri;
    }
    void registerTypes(const char* uri) override
    {
        qInfo() << "[QtAliceVision] Plugin Initialized";
        aliceVision::system::Logger::get()->setLogLevel("info");
        Q_ASSERT(uri == QLatin1String("AliceVision"));

        qmlRegisterType<FeaturesViewer>(uri, 1, 0, "FeaturesViewer");
        qmlRegisterType<MFeatures>(uri, 1, 0, "MFeatures");
        qmlRegisterType<MSfMData>(uri, 1, 0, "MSfMData");
        qmlRegisterType<MTracks>(uri, 1, 0, "MTracks");
        qmlRegisterType<MViewStats>(uri, 1, 0, "MViewStats");
        qmlRegisterType<MSfMDataStats>(uri, 1, 0, "MSfMDataStats");
        qRegisterMetaType<QList<QPointF*>>("QList<QPointF*>");
        qRegisterMetaType<QQmlListProperty<QPointF>>("QQmlListProperty<QPointF>");
        qRegisterMetaType<aliceVision::sfmData::SfMData>(
            "QSharedPtr<aliceVision::sfmData::SfMData>"); // for usage in signals/slots
        qRegisterMetaType<size_t>("size_t");              // for usage in signals/slots

        qmlRegisterType<FloatImageViewer>(uri, 1, 0, "FloatImageViewer");
        qmlRegisterType<Surface>(uri, 1, 0, "Surface");
        qmlRegisterType<PanoramaViewer>(uri, 1, 0, "PanoramaViewer");
        qRegisterMetaType<QPointF>("QPointF");
        qRegisterMetaType<FloatImage>();
        qRegisterMetaType<std::shared_ptr<FloatImage>>();

        qRegisterMetaType<Surface*>("Surface*");

        qRegisterMetaType<imgserve::Response>("Response");
        qRegisterMetaType<imgserve::Response>("imgserve::Response");
    }
};

} // namespace qtAliceVision
