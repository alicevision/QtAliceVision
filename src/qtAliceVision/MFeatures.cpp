#include "MFeatures.hpp"

#include <QDebug>
#include <QThreadPool>
#include <QFileInfo>
#include <QMap>
#include <QString>
#include <QVariant>

#include <aliceVision/feature/ImageDescriber.hpp>
#include <aliceVision/sfm/pipeline/regionsIO.hpp>
#include <aliceVision/sfmData/SfMData.hpp>

#include <algorithm>
#include <memory>
#include <limits>

namespace qtAliceVision
{

void FeaturesIORunnable::run()
{
    FeaturesPerViewPerDesc* featuresPerViewPerDesc = new FeaturesPerViewPerDesc;

    std::vector<aliceVision::feature::EImageDescriberType> imageDescriberTypes;
    for (const auto& descType : _describerTypes)
    {
        imageDescriberTypes.emplace_back(
            aliceVision::feature::EImageDescriberType_stringToEnum(descType));
    }

    std::vector<std::vector<std::unique_ptr<aliceVision::feature::Regions>>> regionsPerViewPerDesc;
    bool loaded
        = aliceVision::sfm::loadFeaturesPerDescPerView(
            regionsPerViewPerDesc, _viewIds, {_folder}, imageDescriberTypes);

    if (!loaded)
    {
        qWarning() << "[QtAliceVision] Features: Failed to load features from folder: " << _folder.c_str();
        delete featuresPerViewPerDesc;
        Q_EMIT resultReady(nullptr);
        return;
    }

    for (int dIdx = 0; dIdx < _describerTypes.size(); ++dIdx)
    {
        const auto& descTypeStr = _describerTypes.at(dIdx);

        const std::vector<std::unique_ptr<aliceVision::feature::Regions>>& regionsPerView =
            regionsPerViewPerDesc.at(static_cast<uint>(dIdx));

        for (std::size_t vIdx = 0; vIdx < _viewIds.size(); ++vIdx)
        {
            const auto& viewId = _viewIds.at(vIdx);

            qDebug() << "[QtAliceVision] Features: Load " << descTypeStr
                     << " from viewId: " << viewId << ".";

            (*featuresPerViewPerDesc)[descTypeStr][viewId] = regionsPerView.at(vIdx)->Features();
        }
    }

    Q_EMIT resultReady(featuresPerViewPerDesc);
}

MFeatures::MFeatures()
{
    // trigger features reload events
    connect(this, &MFeatures::featureFolderChanged, this, &MFeatures::load);
    connect(this, &MFeatures::describerTypesChanged, this, &MFeatures::load);
    connect(this, &MFeatures::viewIdsChanged, this, &MFeatures::load);
}

MFeatures::~MFeatures()
{
    setStatus(None);

    if (_featuresPerViewPerDesc)
    {
        delete _featuresPerViewPerDesc;
    }
}

void MFeatures::load()
{
    _needReload = false;

    if (_status == Loading)
    {
        qDebug("[QtAliceVision] Features: Unable to load, a load event is already running.");
        _needReload = true;
        return;
    }

    if (_status == Loading)
    {
        qDebug("[QtAliceVision] Features: Unable to load, a load event is already running.");
        return;
    }

    if (_describerTypes.empty())
    {
        qDebug("[QtAliceVision] Features: Unable to load, no describer types given.");
        setStatus(None);
        return;
    }

    if (_featureFolder.isEmpty())
    {
        qDebug("[QtAliceVision] Features: Unable to load, no feature folder given.");
        setStatus(None);
        return;
    }

    if (_viewIds.empty())
    {
        // no need to load features in a seperate thread (e.g. data already in memory).
        // call onFeaturesReady because Tracks / SfMData information may need to be updated.
        onFeaturesReady(nullptr);
        return;
    }

    // load features from file in a seperate thread
    qDebug("[QtAliceVision] Features: Load features from file in a seperate thread.");

    std::string featureFolder = _featureFolder.toLocalFile().toStdString();

    std::vector<aliceVision::IndexT> viewIds;
    for (const auto& var : _viewIds)
    {
        viewIds.push_back(static_cast<aliceVision::IndexT>(var.toUInt()));
    }

    std::vector<std::string> describerTypes;
    for (const auto& var : _describerTypes)
    {
        describerTypes.push_back(var.toString().toStdString());
    }

    FeaturesIORunnable* ioRunnable =
        new FeaturesIORunnable(featureFolder, viewIds, describerTypes);
    
    connect(ioRunnable, &FeaturesIORunnable::resultReady, this, &MFeatures::onFeaturesReady);

    QThreadPool::globalInstance()->start(ioRunnable);
}

void MFeatures::onFeaturesReady(FeaturesPerViewPerDesc* featuresPerViewPerDesc)
{
    if (_needReload)
    {
        setStatus(None);
        load();
        return;
    }

    if (featuresPerViewPerDesc)
    {
        if (_featuresPerViewPerDesc)
        {
            delete _featuresPerViewPerDesc;
        }

        _featuresPerViewPerDesc = featuresPerViewPerDesc;
        updateMinMaxFeatureScale();
        updateFeaturesInfo();
    }

    setStatus(Ready);
}

void MFeatures::setStatus(Status status)
{
    if (status == _status)
        return;
    _status = status;

    Q_EMIT statusChanged(_status);

    if (status == Ready || status == Error)
    {
        Q_EMIT featuresChanged();
    }
}

void MFeatures::updateMinMaxFeatureScale()
{
    _minFeatureScalePerDesc.clear();
    _maxFeatureScalePerDesc.clear();

    if (!_featuresPerViewPerDesc)
    {
        return;
    }

    for (const auto& [descType, featuresPerView] : *_featuresPerViewPerDesc)
    {
        float scale_min = std::numeric_limits<float>::max();
        float scale_max = 0.f;

        for (const auto& [_, features] : featuresPerView)
        {
            for (const auto& feature : features)
            {
                scale_min = std::min(scale_min, feature.scale());
                scale_max = std::max(scale_max, feature.scale());
            }
        }

        _minFeatureScalePerDesc[descType] = scale_min;
        _maxFeatureScalePerDesc[descType] = scale_max;
    }
}

void MFeatures::updateFeaturesInfo()
{
    qDebug("[QtAliceVision] Features: Update UI features information.");

    _featuresInfo.clear();

    if (!_featuresPerViewPerDesc)
    {
        return;
    }

    for (const auto& [descType, featuresPerView] : *_featuresPerViewPerDesc)
    {
        QMap<QString, QVariant> infoPerView;

        for (const auto& [viewId, features] : featuresPerView)
        {
            QMap<QString, QVariant> info;
            info.insert("nbFeatures", QString::number(features.size()));

            infoPerView.insert(QString::number(viewId), QVariant(info));
        }

        _featuresInfo.insert(QString::fromStdString(descType), QVariant(infoPerView));
    }

    Q_EMIT featuresInfoChanged();
}

} // namespace qtAliceVision

#include "MFeatures.moc"
