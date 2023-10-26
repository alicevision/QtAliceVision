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

namespace qtAliceVision {

void FeaturesIORunnable::run()
{
    FeaturesPerViewPerDesc* featuresPerViewPerDesc = new FeaturesPerViewPerDesc;

    std::vector<aliceVision::feature::EImageDescriberType> imageDescriberTypes;
    for (const auto& descType : _describerTypes)
    {
        imageDescriberTypes.emplace_back(aliceVision::feature::EImageDescriberType_stringToEnum(descType));
    }

    std::vector<std::vector<std::unique_ptr<aliceVision::feature::Regions>>> regionsPerViewPerDesc;
    bool loaded = aliceVision::sfm::loadFeaturesPerDescPerView(regionsPerViewPerDesc, _viewIds, _folders, imageDescriberTypes);

    if (!loaded)
    {
        qWarning() << "[QtAliceVision] Features: Failed to load features";
        delete featuresPerViewPerDesc;
        Q_EMIT resultReady(nullptr);
        return;
    }

    for (std::size_t dIdx = 0; dIdx < _describerTypes.size(); ++dIdx)
    {
        const auto& descTypeStr = _describerTypes.at(dIdx);

        const std::vector<std::unique_ptr<aliceVision::feature::Regions>>& regionsPerView = regionsPerViewPerDesc.at(static_cast<uint>(dIdx));

        for (std::size_t vIdx = 0; vIdx < _viewIds.size(); ++vIdx)
        {
            const auto& viewId = _viewIds.at(vIdx);

            qDebug() << "[QtAliceVision] Features: Load " << descTypeStr << " from viewId: " << viewId << ".";

            (*featuresPerViewPerDesc)[descTypeStr][viewId] = regionsPerView.at(vIdx)->Features();
        }
    }

    Q_EMIT resultReady(featuresPerViewPerDesc);
}

MFeatures::MFeatures()
{
    // trigger features reload events
    connect(this, &MFeatures::featureFoldersChanged, this, &MFeatures::load);
    connect(this, &MFeatures::describerTypesChanged, this, &MFeatures::load);
    connect(this, &MFeatures::viewIdsChanged, this, &MFeatures::load);
}

MFeatures::~MFeatures()
{
    if (_featuresPerViewPerDesc)
        delete _featuresPerViewPerDesc;

    setStatus(None);
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

    if (_describerTypes.empty())
    {
        qDebug("[QtAliceVision] Features: Unable to load, no describer types given.");
        setStatus(None);
        return;
    }

    if (_featureFolders.empty())
    {
        qDebug("[QtAliceVision] Features: Unable to load, no feature folder given.");
        setStatus(None);
        return;
    }

    if (_viewIds.empty())
    {
        qDebug("[QtAliceVision] Features: Unable to load, no viewId given.");
        setStatus(None);
        return;
    }

    setStatus(Loading);

    // load features from file in a seperate thread
    qDebug("[QtAliceVision] Features: Load features from file in a seperate thread.");

    std::vector<std::string> folders;
    for (const auto& var : _featureFolders)
    {
        folders.push_back(var.toString().toStdString());
    }

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

    FeaturesIORunnable* ioRunnable = new FeaturesIORunnable(folders, viewIds, describerTypes);

    connect(ioRunnable, &FeaturesIORunnable::resultReady, this, &MFeatures::onFeaturesReady);

    QThreadPool::globalInstance()->start(ioRunnable);
}

void MFeatures::onFeaturesReady(FeaturesPerViewPerDesc* featuresPerViewPerDesc)
{
    if (_needReload)
    {
        if (featuresPerViewPerDesc)
            delete featuresPerViewPerDesc;

        setStatus(None);
        load();
        return;
    }

    if (featuresPerViewPerDesc)
    {
        if (_featuresPerViewPerDesc)
            delete _featuresPerViewPerDesc;

        _featuresPerViewPerDesc = featuresPerViewPerDesc;
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

int MFeatures::nbFeatures(QString describerType, int viewId) const
{
    if (_status != Ready)
    {
        return 0;
    }

    if (!_featuresPerViewPerDesc)
    {
        return 0;
    }

    const auto featuresPerViewIt = _featuresPerViewPerDesc->find(describerType.toStdString());
    if (featuresPerViewIt == _featuresPerViewPerDesc->end())
    {
        return 0;
    }

    if (viewId < 0)
    {
        return 0;
    }

    const auto& featuresPerView = featuresPerViewIt->second;
    const auto featuresIt = featuresPerView.find(static_cast<aliceVision::IndexT>(viewId));
    if (featuresIt == featuresPerView.end())
    {
        return 0;
    }

    const auto& features = featuresIt->second;
    return static_cast<int>(features.size());
}

}  // namespace qtAliceVision

#include "MFeatures.moc"
