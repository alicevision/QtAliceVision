#include "MFeatures.hpp"

#include <QThreadPool>

namespace qtAliceVision 
{

MFeatures::MFeatures(QObject* parent):
    QObject(parent)
{
    setupBasicConnections();
}

MFeatures::~MFeatures()
{
    _allFeatures.clear(); // Make sure to clear the Map and free everything inside it 
}

void MFeatures::setupBasicConnections()
{
    // trigger features reload events
    connect(this, &MFeatures::describerTypesChanged, this, &MFeatures::reloadQMap);
    connect(this, &MFeatures::featureFolderChanged, this, &MFeatures::updateMapFeatureFolder);
    connect(this, &MFeatures::viewIdChanged, this, &MFeatures::updateMapViewId);

    // trigger SFM and Tracks updates
    connect(this, &MFeatures::tracksChanged, this, &MFeatures::updateFeatureFromTracksEmit);
    connect(this, &MFeatures::sfmDataChanged, this, &MFeatures::updateFeatureFromSfMEmit);
}

void MFeatures::reloadQMap()
{
    // Not sure it is really useful (because we clear the map later) but just in case
    for (auto i = _allFeatures.constBegin(); i != _allFeatures.constEnd(); ++i)
    {
        disconnect(this, &MFeatures::featuresChanged, i.value().get(), &MDescFeatures::featuresReadyToDisplayChanged);

        disconnect(i.value().get(), &MDescFeatures::featuresReadyChanged, this, &MFeatures::updateFeatures);
        disconnect(i.value().get(), &MDescFeatures::loadingFeaturesChanged, this, &MFeatures::updateLoadingFeatures);
    }

    _allFeatures.clear(); // Clear the map and delete the MDescFeatures instances

    // Populate the map and connect signals to slots
    for (const QString& desc : _describerTypes)
    {
        QSharedPointer<MDescFeatures> descFeatures(new MDescFeatures(desc, _featureFolder, _viewId));
        _allFeatures.insert(desc, descFeatures);

        connect(this, &MFeatures::featuresChanged, descFeatures.get(), &MDescFeatures::featuresReadyToDisplayChanged);

        connect(descFeatures.get(), &MDescFeatures::featuresReadyChanged, this, &MFeatures::updateFeatures);
        connect(descFeatures.get(), &MDescFeatures::loadingFeaturesChanged, this, &MFeatures::updateLoadingFeatures);
    }

    Q_EMIT mapChanged();
}

void MFeatures::updateMapFeatureFolder()
{
    for (auto i = _allFeatures.constBegin(); i != _allFeatures.constEnd(); ++i)
    {
        i.value()->setFeatureFolder(_featureFolder);
    }
}

void MFeatures::updateMapViewId()
{
    for (auto i = _allFeatures.constBegin(); i != _allFeatures.constEnd(); ++i)
    {
        i.value()->setViewId(_viewId);
    }
}
   
void MFeatures::setMTracks(MTracks* tracks)
{
    if (_mtracks == tracks)
        return;
    if (_mtracks != nullptr)
    {
        disconnect(_mtracks, &MTracks::tracksChanged, this, &MFeatures::tracksChanged);
    }
    _mtracks = tracks;
    if (_mtracks != nullptr)
    {
        connect(_mtracks, &MTracks::tracksChanged, this, &MFeatures::tracksChanged);
    }

    Q_EMIT tracksChanged();
}

void MFeatures::setMSfmData(MSfMData* sfmData)
{
    if (_msfmData == sfmData)
        return;
    if (_msfmData != nullptr)
    {
        disconnect(_msfmData, &MSfMData::sfmDataChanged, this, &MFeatures::sfmDataChanged);
    }
    _msfmData = sfmData;
    if (_msfmData != nullptr)
    {
        connect(_msfmData, &MSfMData::sfmDataChanged, this, &MFeatures::sfmDataChanged);
    }

    Q_EMIT sfmDataChanged();
}

void MFeatures::updateFeatures(const QString& desc)
{
    updateFeatureFromTracks(desc);
    updateFeatureFromSfM(desc);
    _allFeatures.value(desc)->setReady(true);
    reloadFeatures();
}

void MFeatures::reloadFeatures()
{
    if (!checkAllFeaturesReady()) return; // Return if at least one of the MDescFeatures is not ready

    updateNbTracks();
    updateTotalNbLandmarks();

    Q_EMIT featuresChanged();
}

void MFeatures::clearTracksFromFeatures(const QList<MFeature*>& features)
{
    for (const auto feature : features)
    {
        feature->clearTrack();
    }
}

void MFeatures::updateFeatureFromTracks(const QString& desc)
{
    QSharedPointer<MDescFeatures> descFeatures = _allFeatures.value(desc);
    const QList<MFeature*>& features = descFeatures->getFeaturesData();

    if (features.empty())
        return;
    clearTracksFromFeatures(features);

    if (_mtracks == nullptr)
    {
        qWarning() << "[QtAliceVision] MFeatures::updateFeatureFromTracks: no Track";
        Q_EMIT descFeatures->updateDisplayTracks(false);
        return;
    }
    if (_mtracks->status() != MTracks::Ready)
    {
        qWarning() << "[QtAliceVision] MFeatures::updateFeatureFromTracks: Tracks is not ready: " << _mtracks->status();
        Q_EMIT descFeatures->updateDisplayTracks(false);
        return;
    }
    if (_mtracks->tracks().empty())
    {
        qWarning() << "[QtAliceVision] MFeatures::updateFeatureFromTracks: Tracks is empty";
        Q_EMIT descFeatures->updateDisplayTracks(false);
        return;
    }

    // Update newly loaded features with information from the sfmData
    aliceVision::feature::EImageDescriberType descType = aliceVision::feature::EImageDescriberType_stringToEnum(desc.toStdString());

    auto tracksPerViewIt = _mtracks->tracksPerView().find(_viewId);
    if (tracksPerViewIt == _mtracks->tracksPerView().end())
    {
        qWarning() << "[QtAliceVision] MFeatures::updateFeatureFromTracks: view does not exist in tracks.";
        return;
    }
    const auto& tracksInCurrentView = tracksPerViewIt->second;
    const auto& tracks = _mtracks->tracks();
    for (const auto& trackId : tracksInCurrentView)
    {
        const auto& trackIterator = tracks.find(trackId);
        if (trackIterator == tracks.end())
        {
            qWarning() << "[QtAliceVision] MFeatures::updateFeatureFromTracks: Track id: " << trackId << " in current view does not exist in Tracks.";
            continue;
        }
        const aliceVision::track::Track& currentTrack = trackIterator->second;

        if (currentTrack.descType != descType)
            continue;

        const auto& featIdPerViewIt = currentTrack.featPerView.find(_viewId);
        if (featIdPerViewIt == currentTrack.featPerView.end())
        {
            qWarning() << "[QtAliceVision] MFeatures::updateFeatureFromTracks: featIdPerViewIt invalid.";
            continue;
        }
        const std::size_t featId = featIdPerViewIt->second;
        if (featId >= features.size())
        {
            qWarning() << "[QtAliceVision] MFeatures::updateFeatureFromTracks: featId invalid regarding loaded features.";
            continue;
        }
        MFeature* feature = features.at(featId);
        feature->setTrackId(trackId);
    }

    //Q_EMIT descFeatures->updateDisplayTracks(true);
}

void MFeatures::updateFeatureFromTracksEmit()
{
    // Iterate through every MDescFeatures
    for (auto i = _allFeatures.constBegin(); i != _allFeatures.constEnd(); ++i)
    {
        QString desc = i.key();
        updateFeatureFromTracks(desc);
    }
        
    updateNbTracks();
    updateTotalNbLandmarks();

    Q_EMIT featuresChanged();
}

void MFeatures::updateTotalNbLandmarks()
{
    _nbLandmarks = 0;

    // Iterate through every MDescFeatures
    for (auto i = _allFeatures.constBegin(); i != _allFeatures.constEnd(); ++i)
    {
        QSharedPointer<MDescFeatures> descFeatures = i.value();
        _nbLandmarks += descFeatures->getNbLandmarks();
    }
}

void MFeatures::updateNbTracks()
{
    _nbTracks = 0; // Total number of tracks

    // Iterate through every MDescFeatures
    for (auto i = _allFeatures.constBegin(); i != _allFeatures.constEnd(); ++i)
    {
        unsigned int nbTracksPerDesc = 0; // Local var to count

        QSharedPointer<MDescFeatures> descFeatures = i.value();
        descFeatures->setNbTracks(0);

        for (MFeature* feature : descFeatures->getFeaturesData())
        {
            if (feature->trackId() < 0)
                continue;
            ++nbTracksPerDesc;
        }
        descFeatures->setNbTracks(nbTracksPerDesc); // Set the number of tracks to each MDescFeatures
        _nbTracks += nbTracksPerDesc; // Increment the total number of tracks
    }
}

void MFeatures::clearSfMFromFeatures(const QList<MFeature*>& features)
{
    for (const auto feature : features)
    {
        feature->clearLandmarkInfo();
    }
}

void MFeatures::updateFeatureFromSfM(const QString& desc)
{
    const QList<MFeature*>& features = _allFeatures.value(desc)->getFeaturesData();

    clearSfMFromFeatures(features);

    unsigned int nbLandmarks = 0; // Local variable to count the number of landmarks for those features

    if (features.empty())
        return;
    if (_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MFeatures::updateFeatureFromSfM: no SfMData";
        return;
    }
    if (_msfmData->status() != MSfMData::Ready)
    {
        qWarning() << "[QtAliceVision] MFeatures::updateFeatureFromSfM: SfMData is not ready: " << _msfmData->status();
        return;
    }
    if (_msfmData->rawData().getViews().empty())
    {
        qWarning() << "[QtAliceVision] MFeatures::updateFeatureFromSfM: SfMData is empty";
        return;
    }
    const auto viewIt = _msfmData->rawData().getViews().find(_viewId);
    if (viewIt == _msfmData->rawData().getViews().end())
    {
        qWarning() << "[QtAliceVision] MFeatures::updateFeatureFromSfM: View " << _viewId << " is not is the SfMData";
        return;
    }
    const aliceVision::sfmData::View& view = *viewIt->second;
    if (!_msfmData->rawData().isPoseAndIntrinsicDefined(&view))
    {
        qWarning() << "[QtAliceVision] MFeatures::updateFeatureFromSfM: SfMData has no valid pose and intrinsic for view " << _viewId;
        return;
    }

    // Update newly loaded features with information from the sfmData
    aliceVision::feature::EImageDescriberType descType = aliceVision::feature::EImageDescriberType_stringToEnum(desc.toStdString());

    const aliceVision::sfmData::CameraPose pose = _msfmData->rawData().getPose(view);
    const aliceVision::geometry::Pose3 camTransform = pose.getTransform();
    const aliceVision::camera::IntrinsicBase* intrinsic = _msfmData->rawData().getIntrinsicPtr(view.getIntrinsicId());

    int numLandmark = 0;
    for (const auto& landmark : _msfmData->rawData().getLandmarks())
    {
        if (landmark.second.descType != descType)
            continue;

        auto itObs = landmark.second.observations.find(_viewId);
        if (itObs != landmark.second.observations.end())
        {
            // setup landmark id and landmark 2d reprojection in the current view
            aliceVision::Vec2 r = intrinsic->project(camTransform, landmark.second.X);

            if (itObs->second.id_feat >= 0 && itObs->second.id_feat < features.size())
            {
                features.at(itObs->second.id_feat)->setLandmarkInfo(landmark.first, r.cast<float>());
            }
            else if (!features.empty())
            {
                qWarning() << "[QtAliceVision] ---------- ERROR id_feat: " << itObs->second.id_feat << ", size: " << features.size();
            }

            ++nbLandmarks;
        }
        ++numLandmark;
    }

    _allFeatures.value(desc)->setNbLandmarks(nbLandmarks); // Set the number of landmarks to the good MDescFeatures
}

void MFeatures::updateFeatureFromSfMEmit()
{
    for (auto i = _allFeatures.constBegin(); i != _allFeatures.constEnd(); ++i)
    {
        QString desc = i.key();
        updateFeatureFromSfM(desc);
    }
    updateNbTracks();
    updateTotalNbLandmarks();

    Q_EMIT featuresChanged();
}

} // namespace