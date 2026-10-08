#pragma once

#include "CurveAxisTicks.hpp"
#include "CurveBandModel.hpp"
#include "CurveModel.hpp"
#include "CurveViewer.hpp"

#include <QtQml/QQmlExtensionPlugin>
#include <QtQml/QtQml>

namespace curveViewer {

class CurveViewerPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "curveViewer.qmlPlugin")

  public:
    void initializeEngine([[maybe_unused]] QQmlEngine* engine, [[maybe_unused]] const char* uri) override {}

    void registerTypes(const char* uri) override
    {
        Q_ASSERT(uri == QLatin1String("CurveViewer"));
        qmlRegisterType<CurveModel>(uri, 1, 0, "CurveModel");
        qmlRegisterType<CurveBandModel>(uri, 1, 0, "CurveBandModel");
        qmlRegisterType<CurveViewer>(uri, 1, 0, "CurveViewer");
        qmlRegisterUncreatableType<CurveAxisTicks>(uri, 1, 0, "CurveAxisTicks", "CurveAxisTicks is provided by CurveViewer");
    }
};

}  // namespace curveViewer
