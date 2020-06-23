#include "MDescFeatures.hpp"

#include <QThreadPool>
#include <QDebug>

namespace qtAliceVision
{

MDescFeatures::MDescFeatures(const QString& desc, const QUrl& folder, aliceVision::IndexT viewId, QObject* parent) :
    _describerType(desc), _featureFolder(folder), _viewId(viewId), QObject(parent)
{
}

MDescFeatures::~MDescFeatures()
{
    qDeleteAll(_features);
}

void MDescFeatures::reloadFeatures()
{
    _nbLandmarks = 0;
    setReady(false);

    // Make sure to free all the MFeature pointers and to clear the QList.
    qDeleteAll(_features);
    _features.clear();
    Q_EMIT featuresChanged();

    _outdatedFeatures = false;

    if (!_featureFolder.isValid() || _viewId == aliceVision::UndefinedIndexT)
    {
        qWarning() << "[QtAliceVision] MDescFeatures::reloadFeatures: No valid folder or view. Describer : " << _describerType;
        Q_EMIT featuresChanged();
        return;
    }

    if (!_loadingFeatures)
    {
        setLoadingFeatures(true);

        // load features from file in a seperate thread
        auto* ioRunnable = new FeatureIORunnable(FeatureIORunnable::IOParams(_featureFolder, _viewId, _describerType));
        connect(ioRunnable, &FeatureIORunnable::resultReady, this, &MDescFeatures::onFeaturesResultReady);
        QThreadPool::globalInstance()->start(ioRunnable);
    }
    else
    {
        // mark current request as outdated
        _outdatedFeatures = true;
    }
}

void MDescFeatures::onFeaturesResultReady(QList<MFeature*> features)
{
    // another request has been made while io thread was working
    if (_outdatedFeatures)
    {
        // clear result and reload features from file with current parameters
        qDeleteAll(features);
        setLoadingFeatures(false);
        reloadFeatures();
        return;
    }

    // update features
    _features = features;
    setLoadingFeatures(false);

    Q_EMIT featuresReadyChanged(_describerType); // Call the updateFeatures slot on the MFeatures instance
    Q_EMIT featuresChanged();
}

} // namespace