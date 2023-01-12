#include "MFeature.hpp"

#include <QSGGeometryNode>
#include <QSGGeometry>
#include <QSGVertexColorMaterial>
#include <QSGFlatColorMaterial>

#include <QtMath>
#include <QThreadPool>
#include <QTransform>

#include <aliceVision/feature/PointFeature.hpp>
#include <aliceVision/feature/ImageDescriber.hpp>
#include <aliceVision/sfm/pipeline/regionsIO.hpp>

#include <aliceVision/sfmDataIO/sfmDataIO.hpp>


namespace qtAliceVision
{

void FeatureIORunnable::run()
{
    using namespace aliceVision;

    // unpack parameters
    QUrl folder;
    aliceVision::IndexT viewId;
    QString descType;
    std::tie(folder, viewId, descType) = _params;

    std::unique_ptr<aliceVision::feature::Regions> regions;
    QList<MFeature*> feats;
    try
    {
        std::unique_ptr<feature::ImageDescriber> describer =
            feature::createImageDescriber(feature::EImageDescriberType_stringToEnum(descType.toStdString()));
        regions = sfm::loadFeatures({folder.toLocalFile().toStdString()}, viewId, *describer);
    }
    catch(std::exception& e)
    {
        qDebug() << "[QtAliceVision] Failed to load features (" << descType << ") for view: " << viewId
                 << " from folder: " << folder << "\n" << e.what();

        Q_EMIT resultReady(feats);
        return;
    }

    feats.reserve(static_cast<int>(regions->RegionCount()));
    for(const auto& f : regions->Features())
    {
        feats.append(new MFeature(f));
    }
    Q_EMIT resultReady(feats);
}

}
