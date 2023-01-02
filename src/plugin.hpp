#pragma once

#include "FeaturesViewer.hpp"
#include "FloatImageViewer.hpp"
#include "PanoramaViewer.hpp"
#include "Surface.hpp"
#include "MViewStats.hpp"
#include "MSfMDataStats.hpp"
#include "MTracks.hpp"
#include "MFeatures.hpp"
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
        qmlRegisterType<MFeatures>(uri, 1, 0, "MFeatures");
        qmlRegisterType<MSfMData>(uri, 1, 0, "MSfMData");
        qmlRegisterType<MTracks>(uri, 1, 0, "MTracks");
        qmlRegisterType<MViewStats>(uri, 1, 0, "MViewStats");
        qmlRegisterType<MSfMDataStats>(uri, 1, 0, "MSfMDataStats");
        qmlRegisterUncreatableType<MFeature>(uri, 1, 0, "MFeature", "Cannot create Feature instances from QML.");
        qRegisterMetaType<QList<MFeature*>>( "QList<MFeature*>" ); // for usage in signals/slots
        qRegisterMetaType<QList<QPointF*>>("QList<QPointF*>");
        qRegisterMetaType<QQmlListProperty<QPointF>>("QQmlListProperty<QPointF>");
        qRegisterMetaType<aliceVision::sfmData::SfMData>( "QSharedPtr<aliceVision::sfmData::SfMData>" ); // for usage in signals/slots
        qRegisterMetaType<size_t>("size_t"); // for usage in signals/slots

        qmlRegisterType<FloatImageViewer>(uri, 1, 0, "FloatImageViewer");
        qmlRegisterType<Surface>(uri, 1, 0, "Surface");
        qmlRegisterType<PanoramaViewer>(uri, 1, 0, "PanoramaViewer");
        qRegisterMetaType<QPointF>("QPointF");
        qRegisterMetaType<FloatImage>();
        qRegisterMetaType<QSharedPointer<FloatImage>>();

        qRegisterMetaType<Surface*>("Surface*");
    }
};

} // namespace
