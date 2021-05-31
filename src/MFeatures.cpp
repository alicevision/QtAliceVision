#include "MFeatures.hpp"

#include <QThreadPool>
#include <QMap>

#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/feature/ImageDescriber.hpp>
#include <aliceVision/sfm/pipeline/regionsIO.hpp>

namespace qtAliceVision
{

/**
 * @brief QRunnable object dedicated to load features using AliceVision.
 */
  class FeaturesIORunnable : public QObject, public QRunnable
  {
    Q_OBJECT

  public:
    /// io parameters: folder, viewIds, describerType
    using IOParams = std::tuple<QUrl, std::vector<aliceVision::IndexT>, QVariantList>;

    explicit FeaturesIORunnable(const IOParams& params)
      : _params(params)
    {}

    /// Load features based on input parameters
    Q_SLOT void run() override;

    /**
     * @brief  Emitted when features have been loaded and Features objects created.
     * @warning Features objects are not parented - their deletion must be handled manually.
     *
     * @param features the loaded Features list
     */
    Q_SIGNAL void resultReady(MFeatures::MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc);

  private:
    IOParams _params;
  };

void FeaturesIORunnable::run()
{
    using namespace aliceVision;

    // unpack parameters
    QUrl folder;
    std::vector<aliceVision::IndexT> viewIds;
    QVariantList descTypes;
    std::tie(folder, viewIds, descTypes) = _params;


    std::vector<std::vector<std::unique_ptr<aliceVision::feature::Regions>>> regionsPerViewPerDesc;
    std::unique_ptr<MFeatures::MViewFeaturesPerViewPerDesc> viewFeaturesPerViewPerDescPtr(new MFeatures::MViewFeaturesPerViewPerDesc);

    std::vector<feature::EImageDescriberType> imageDescriberTypes;
    for (const auto& descType : descTypes)
    {
      imageDescriberTypes.emplace_back(feature::EImageDescriberType_stringToEnum(descType.toString().toStdString()));
    }

    bool loaded = sfm::loadFeaturesPerDescPerView(regionsPerViewPerDesc, viewIds, {folder.toLocalFile().toStdString()}, imageDescriberTypes);

    if(!loaded)
    {
        qWarning() << "[QtAliceVision] Features: Failed to load features from folder: " << folder;
        Q_EMIT resultReady(viewFeaturesPerViewPerDescPtr.release());
        return;
    }

    for (int dIdx = 0; dIdx < descTypes.size(); ++dIdx)
    {
      std::vector<std::unique_ptr<aliceVision::feature::Regions>>& regionsPerView = regionsPerViewPerDesc.at(dIdx);

      for (int vIdx = 0; vIdx < viewIds.size(); ++vIdx)
      {
        qDebug() << "[QtAliceVision] Features: Load " << descTypes.at(dIdx).toString() << " from viewId: " << viewIds.at(vIdx) << ".";

        MFeatures::MViewFeatures& featuresPerView = (*viewFeaturesPerViewPerDescPtr)[descTypes.at(dIdx).toString()][viewIds.at(vIdx)];
        featuresPerView.features.reserve(static_cast<int>(regionsPerView.at(vIdx)->RegionCount()));

        for (const auto& f : regionsPerView.at(vIdx)->Features())
          featuresPerView.features.append(new MFeature(f));
      }
    }

    Q_EMIT resultReady(viewFeaturesPerViewPerDescPtr.release());
}

MFeatures::MFeatures()
{
  // trigger features reload events
  connect(this, &MFeatures::featureFolderChanged, this, &MFeatures::load);
  connect(this, &MFeatures::describerTypesChanged, this, &MFeatures::load);
  connect(this, &MFeatures::currentViewIdChanged, this, &MFeatures::load);
  connect(this, &MFeatures::loadTimeWindowChanged, this, &MFeatures::load);
  connect(this, &MFeatures::timeWindowChanged, this, &MFeatures::load);
  connect(this, &MFeatures::tracksChanged, this, &MFeatures::load);
  connect(this, &MFeatures::sfmDataChanged, this, &MFeatures::load);
}

MFeatures::~MFeatures()
{
  setStatus(None);
  clearAll();
}

void MFeatures::setMTracks(MTracks* tracks)
{
  if (_mtracks == tracks)
    return;

  if (_mtracks != nullptr)
  {
    disconnect(_mtracks, SIGNAL(tracksChanged()), this, SIGNAL(tracksChanged()));
  }

  _mtracks = tracks;

  if (_mtracks != nullptr)
  {
    connect(_mtracks, SIGNAL(tracksChanged()), this, SIGNAL(tracksChanged()));
  }
  Q_EMIT tracksChanged();
}

void MFeatures::setMSfmData(MSfMData* sfmData)
{
  if (_msfmData == sfmData)
    return;

  if (_msfmData != nullptr)
  {
    disconnect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
  }

  _msfmData = sfmData;

  if (_msfmData != nullptr)
  {
    connect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
  }
  Q_EMIT sfmDataChanged();
}

void MFeatures::load()
{
  _outdatedFeatures = false;

  if (status() == Loading)
  {
    qDebug("[QtAliceVision] Features: Unable to load, a load event is already running.");
    _outdatedFeatures = true;
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

  if(_currentViewId == aliceVision::UndefinedIndexT)
  {
    setStatus(None);
    return;
  }

  setStatus(Loading);

  std::vector<aliceVision::IndexT> viewIds;

  getViewIdsToLoad(viewIds);

  if (viewIds.empty())
  {
    // no need to load features in a seperate thread (e.g. data already in memory).
    // call onFeaturesReady because Tracks / SfMData information may need to be updated.
    onFeaturesReady(nullptr);
    return;
  }

  qDebug("[QtAliceVision] Features: Load features from file in a seperate thread.");

  // load features from file in a seperate thread
  FeaturesIORunnable* ioRunnable = new FeaturesIORunnable(FeaturesIORunnable::IOParams(_featureFolder, viewIds, _describerTypes));
  connect(ioRunnable, &FeaturesIORunnable::resultReady, this, &MFeatures::onFeaturesReady);
  QThreadPool::globalInstance()->start(ioRunnable);
}

void MFeatures::onFeaturesReady(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc)
{
  if (_outdatedFeatures) 
  {
    clearViewFeaturesPerViewPerDesc(viewFeaturesPerViewPerDesc); // handle viewFeaturesPerViewPerDesc == nullptr case
    setStatus(None);
    load();
    return;
  }

  if (viewFeaturesPerViewPerDesc != nullptr)
  {
    for (auto& viewFeaturesPerViewPerDescPair : *viewFeaturesPerViewPerDesc)
    {
      const auto& describerType = viewFeaturesPerViewPerDescPair.first;
      (*_viewFeaturesPerViewPerDesc)[describerType].insert(viewFeaturesPerViewPerDescPair.second.begin(), viewFeaturesPerViewPerDescPair.second.end());
    }
  }

  updateFromTracks(_viewFeaturesPerViewPerDesc);
  updateFromSfM(_viewFeaturesPerViewPerDesc);
  updateFeaturesInfo();

  setStatus(Ready);
}

aliceVision::IndexT MFeatures::getCurrentFrameId() const
{
  aliceVision::IndexT currentFrameId = aliceVision::UndefinedIndexT;

  if (_msfmData != nullptr)
  {
    try
    {
      currentFrameId = _msfmData->rawData().getView(_currentViewId).getFrameId();
    }
    catch (std::exception& e)
    {
      qWarning() << "[QtAliceVision] Features: Failed to get current frame id."<< "\n" << e.what();
    }
  }

  return currentFrameId;
}

const MFeatures::MViewFeatures* MFeatures::getCurrentViewFeatures(const QString& describerType) const
{
  if (_viewFeaturesPerViewPerDesc == nullptr)
    return nullptr;

  auto itrDesc = _viewFeaturesPerViewPerDesc->find(describerType);
  if (itrDesc == _viewFeaturesPerViewPerDesc->end())
    return nullptr;

  auto itrView = itrDesc->second.find(_currentViewId);
  if (itrView == itrDesc->second.end())
    return nullptr;

  return &(itrView->second);
}

void MFeatures::getViewFeaturesPerFramePerTrack(const QString& describerType, MViewFeaturesPerFramePerTrack& featurePerFramePerTrack) const
{
  if (_viewFeaturesPerViewPerDesc == nullptr)
    return;

  auto itrDesc = _viewFeaturesPerViewPerDesc->find(describerType);
  if (itrDesc == _viewFeaturesPerViewPerDesc->end())
    return;

  const MViewFeaturesPerView& featuresPerView = itrDesc->second;

  for (const auto& featuresPerViewPair : featuresPerView)
  {
    const aliceVision::IndexT frameId = featuresPerViewPair.second.frameId;

    if (frameId == aliceVision::UndefinedIndexT)
      continue;

    for (const MFeature* feature : featuresPerViewPair.second.features)
      if (feature->trackId() >= 0)
        featurePerFramePerTrack[feature->trackId()][frameId] = feature;
  }
}

void MFeatures::getViewIdsToLoad(std::vector<aliceVision::IndexT>& viewIdsToLoad)
{
  // multiple views
  if (_loadTimeWindow && _timeWindow > 0 && haveValidLandmarks())
  {
    const aliceVision::sfmData::SfMData& sfmData = _msfmData->rawData();

    aliceVision::IndexT currentIntrinsicId = aliceVision::UndefinedIndexT;
    aliceVision::IndexT currentFrameId = aliceVision::UndefinedIndexT;

    try
    {
      currentIntrinsicId = sfmData.getView(_currentViewId).getIntrinsicId();
      currentFrameId = sfmData.getView(_currentViewId).getFrameId();;
    }
    catch (std::exception& e)
    {
      qDebug() << "[QtAliceVision] Features: Failed to find the current view in the SfM: " << "\n" << e.what();
    }

    if (currentFrameId != aliceVision::UndefinedIndexT)
    {
      const aliceVision::IndexT minFrameId = (currentFrameId < _timeWindow) ? 0 : currentFrameId - _timeWindow;
      const aliceVision::IndexT maxFrameId = currentFrameId + _timeWindow;

      for (const auto& viewPair : sfmData.getViews())
      {
        const aliceVision::sfmData::View& view = *(viewPair.second);
        if (currentIntrinsicId == view.getIntrinsicId())
          if ((view.getFrameId() >= minFrameId) && (view.getFrameId() <= maxFrameId))
            viewIdsToLoad.emplace_back(view.getViewId());
      }
    }
  }

  // single view
  if (viewIdsToLoad.empty())
  {
    viewIdsToLoad.emplace_back(_currentViewId);
  }

  // first initialization
  if (_viewFeaturesPerViewPerDesc == nullptr)
  {
    _viewFeaturesPerViewPerDesc = new MViewFeaturesPerViewPerDesc();
    return;
  }

  // desciber types changed
  if (_viewFeaturesPerViewPerDesc->size() != _describerTypes.size())
  {
    clearViewFeaturesPerViewPerDesc(_viewFeaturesPerViewPerDesc);
    return;
  }

  // caching mechanism
  std::vector<aliceVision::IndexT> viewIdsToKeep;
  std::vector<aliceVision::IndexT> viewIdsToRemove;

  const MViewFeaturesPerView& viewFeaturesPerView = _viewFeaturesPerViewPerDesc->begin()->second;

  // find view id to keep / to remove
  for (const auto& viewFeaturesPerViewPair : viewFeaturesPerView)
  {
    bool toKeep = false;
      
    for (aliceVision::IndexT viewId : viewIdsToLoad)
    {
      if (viewFeaturesPerViewPair.first == viewId)
      {
        viewIdsToKeep.push_back(viewFeaturesPerViewPair.first);
        toKeep = true;
        break;
      }
    }

    if(!toKeep)
      viewIdsToRemove.push_back(viewFeaturesPerViewPair.first);
  }

  if (viewIdsToKeep.empty()) // nothing to do
  {
    clearViewFeaturesPerViewPerDesc(_viewFeaturesPerViewPerDesc);
    return;
  }

  // Remove MViewFeatures in memory
  for (const auto& decriberType : _describerTypes)
  {
    MViewFeaturesPerView& viewFeaturesPerView = _viewFeaturesPerViewPerDesc->at(decriberType.toString());

    for (aliceVision::IndexT viewIdToRemove : viewIdsToRemove)
    {
      auto iter = viewFeaturesPerView.find(viewIdToRemove);

      if (iter != viewFeaturesPerView.end())
        viewFeaturesPerView.erase(iter);
    }
  }

  // Remove view ids kept from view ids to load
  for (aliceVision::IndexT viewId: viewIdsToKeep)
  {
    auto iter = std::find(viewIdsToLoad.begin(), viewIdsToLoad.end(), viewId);

    if (iter != viewIdsToLoad.end())
      viewIdsToLoad.erase(iter);
  }

  qDebug() << "[QtAliceVision] Features: Caching: " << viewIdsToKeep.size() << " frame(s) kept, " 
                                                    << viewIdsToRemove.size() << " frame(s) removed, " 
                                                    << viewIdsToLoad.size() << " frame(s) requested.";
}

void MFeatures::updateFromTracks(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc)
{
  if (viewFeaturesPerViewPerDesc == nullptr || viewFeaturesPerViewPerDesc->empty())
  {
    qDebug("[QtAliceVision] Features: Unable to update from tracks, no features loaded.");
    return;
  }

  if (_mtracks == nullptr)
  {
    qDebug("[QtAliceVision] Features: Unable to update from tracks, no Tracks given");
    return;
  }

  if (_mtracks->status() != MTracks::Ready)
  {
    qDebug() << "[QtAliceVision] Features: Unable to update from tracks, Tracks is not ready: " << _mtracks->status();
    return;
  }

  if (_mtracks->tracks().empty() || _mtracks->tracksPerView().empty())
  {
    qDebug("[QtAliceVision] Features: Unable to update from tracks, Tracks is empty");
    return;
  }

  qDebug("[QtAliceVision] Features: Update from tracks.");

  // Update newly loaded features with information from the sfmData
  for (const auto& describerTypeName : _describerTypes)
  {
    const aliceVision::feature::EImageDescriberType descType = aliceVision::feature::EImageDescriberType_stringToEnum(describerTypeName.toString().toStdString());

    for (auto& viewFeaturesPair : viewFeaturesPerViewPerDesc->at(describerTypeName.toString()))
    {
      const aliceVision::IndexT viewId = viewFeaturesPair.first;
      MViewFeatures& featuresPerView = viewFeaturesPair.second;

      if (featuresPerView.nbTracks > 0) // view already update 
        continue;

      auto tracksPerViewIt = _mtracks->tracksPerView().find(viewId);
      if (tracksPerViewIt == _mtracks->tracksPerView().end())
      {
        qInfo() << "[QtAliceVision] Features: Update from tracks, view: " << viewId << " does not exist in tracks";
        return;
      }

      const auto& tracksInCurrentView = tracksPerViewIt->second;
      const auto& tracks = _mtracks->tracks();

      for (const auto& trackId : tracksInCurrentView)
      {
        const auto& trackIterator = tracks.find(trackId);
        if (trackIterator == tracks.end())
        {
          qInfo() << "[QtAliceVision] Features: Update from tracks, track: " << trackId << " in current view: " << viewId << " does not exist in tracks";
          continue;
        }
        const aliceVision::track::Track& currentTrack = trackIterator->second;

        if (currentTrack.descType != descType)
          continue;

        const auto& featIdPerViewIt = currentTrack.featPerView.find(viewId);
        if (featIdPerViewIt == currentTrack.featPerView.end())
        {
          qInfo() << "[QtAliceVision] Features: Update from tracks, featIdPerViewIt invalid";
          continue;
        }
        const std::size_t featId = featIdPerViewIt->second;
        if (featId >= featuresPerView.features.size())
        {
          qInfo() << "[QtAliceVision] Features: Update from tracks, featId invalid regarding loaded features for view: " << viewId;
          continue;
        }

        MFeature* feature = featuresPerView.features.at(featId);
        feature->setTrackId(trackId);
        ++featuresPerView.nbTracks;
      }
    }
  }
}

void MFeatures::updateFromSfM(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc)
{
  if (viewFeaturesPerViewPerDesc == nullptr || viewFeaturesPerViewPerDesc->empty())
  {
    qDebug("[QtAliceVision] Features: Unable to update from SfM, no features loaded.");
    return;
  }

  if (_msfmData == nullptr)
  {
    qDebug("[QtAliceVision] Features: Unable to update from SfM, no SfMData given.");
    return;
  }

  if (_msfmData->status() != MSfMData::Ready)
  {
    qDebug() << "[QtAliceVision] Features: Unable to update from SfM, SfMData is not ready: " << _msfmData->status();
    return;
  }

  if (_msfmData->rawData().getViews().empty())
  {
    qDebug("[QtAliceVision] Features: Unable to update from SfM, SfMData is empty");
    return;
  }

  qDebug("[QtAliceVision] Features: Update from SfM.");

  for (const auto& describerTypeName : _describerTypes)
  {
    const aliceVision::feature::EImageDescriberType descType = aliceVision::feature::EImageDescriberType_stringToEnum(describerTypeName.toString().toStdString());

    for (auto& viewFeaturesPair : viewFeaturesPerViewPerDesc->at(describerTypeName.toString()))
    {
      const aliceVision::IndexT viewId = viewFeaturesPair.first;
      MViewFeatures& featuresPerView = viewFeaturesPair.second;

      if (featuresPerView.nbLandmarks > 0) // view already update 
        continue;

      const auto viewIt = _msfmData->rawData().getViews().find(viewId);
      if (viewIt == _msfmData->rawData().getViews().end())
      {
        qInfo() << "[QtAliceVision] Features: Update from SfM, view: " << viewId << " is not is the SfMData";
        return;
      }
      const aliceVision::sfmData::View& view = *viewIt->second;

      // Update frame Id
      featuresPerView.frameId = view.getFrameId();

      if (!_msfmData->rawData().isPoseAndIntrinsicDefined(&view))
      {
        qInfo() << "[QtAliceVision] Features: Update from SfM, SfMData has no valid pose and intrinsic for view: " << viewId;
        return;
      }

      // Update newly loaded features with information from the sfmData
      const aliceVision::sfmData::CameraPose pose = _msfmData->rawData().getPose(view);
      const aliceVision::geometry::Pose3 camTransform = pose.getTransform();
      const aliceVision::camera::IntrinsicBase* intrinsic = _msfmData->rawData().getIntrinsicPtr(view.getIntrinsicId());

      for (const auto& landmark : _msfmData->rawData().getLandmarks())
      {
        if (landmark.second.descType != descType)
          continue;

        auto itObs = landmark.second.observations.find(viewId);
        if (itObs != landmark.second.observations.end())
        {
          // setup landmark id and landmark 2d reprojection in the current view
          aliceVision::Vec2 r = intrinsic->project(camTransform, landmark.second.X);

          if (itObs->second.id_feat >= 0 && itObs->second.id_feat < featuresPerView.features.size())
          {
            featuresPerView.features.at(itObs->second.id_feat)->setLandmarkInfo(landmark.first, r.cast<float>());
          }
          else if (!featuresPerView.features.empty())
          {
            qWarning() << "[QtAliceVision] [ERROR] Features: Update from SfM, view: " << viewId << ", id_feat: " << itObs->second.id_feat << ", size: " << featuresPerView.features.size();
          }

          ++featuresPerView.nbLandmarks; // Update nb landmarks
        }
      }
    }
  }
}

void MFeatures::updateFeaturesInfo()
{
  qDebug("[QtAliceVision] Features: Update UI features info.");

  _featuresInfo.clear();

  if (_viewFeaturesPerViewPerDesc == nullptr || _viewFeaturesPerViewPerDesc->empty())
    return;

  for (auto& viewFeaturesPerViewPerDescPair : *_viewFeaturesPerViewPerDesc)
  {
    QMap<QString, QVariant> featuresPerViewInfo;
    for (auto& featuresPerViewPair : viewFeaturesPerViewPerDescPair.second)
    {
      const auto& viewFeatures = featuresPerViewPair.second;
      QMap<QString, QVariant> featuresViewInfo;
      featuresViewInfo.insert("frameId", (viewFeatures.frameId == aliceVision::UndefinedIndexT) ? QString("-") : QString::number(viewFeatures.frameId));
      featuresViewInfo.insert("nbFeatures", QString::number(viewFeatures.features.size()));
      featuresViewInfo.insert("nbTracks", (viewFeatures.nbTracks < 0) ? QString("-") : QString::number(viewFeatures.nbTracks));
      featuresViewInfo.insert("nbLandmarks", (viewFeatures.nbLandmarks < 0) ? QString("-") : QString::number(viewFeatures.nbLandmarks));

      featuresPerViewInfo.insert(QString::number(featuresPerViewPair.first), QVariant(featuresViewInfo));
    }
    _featuresInfo.insert(QString(viewFeaturesPerViewPerDescPair.first), QVariant(featuresPerViewInfo));
  }

  Q_EMIT featuresInfoChanged();
}

void MFeatures::clearViewFeaturesPerViewPerDesc(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc)
{
  if (viewFeaturesPerViewPerDesc == nullptr || viewFeaturesPerViewPerDesc->empty())
    return;

  for (auto& viewFeaturesPerViewPerDescPair : *viewFeaturesPerViewPerDesc)
    for (auto& featuresPerViewPair : viewFeaturesPerViewPerDescPair.second)
      qDeleteAll(featuresPerViewPair.second.features);

  viewFeaturesPerViewPerDesc->clear();
}

void MFeatures::clearAllTrackInfo()
{
  if (_viewFeaturesPerViewPerDesc == nullptr || _viewFeaturesPerViewPerDesc->empty())
    return;

  for (auto& viewFeaturesPerViewPerDescPair : *_viewFeaturesPerViewPerDesc)
  {
    for (auto& featuresPerViewPair : viewFeaturesPerViewPerDescPair.second)
    {
      featuresPerViewPair.second.nbTracks = 0;
      for (MFeature* feature : featuresPerViewPair.second.features)
        feature->clearTrack();
    }
  }
}

void MFeatures::clearAllSfMInfo()
{
  if (_viewFeaturesPerViewPerDesc == nullptr || _viewFeaturesPerViewPerDesc->empty())
    return;

  for (auto& viewFeaturesPerViewPerDescPair : *_viewFeaturesPerViewPerDesc)
  {
    for (auto& featuresPerViewPair : viewFeaturesPerViewPerDescPair.second)
    {
      featuresPerViewPair.second.nbLandmarks = 0;
      for (MFeature* feature : featuresPerViewPair.second.features)
        feature->clearLandmarkInfo();
    }
  }
}

void MFeatures::clearAll()
{
  clearViewFeaturesPerViewPerDesc(_viewFeaturesPerViewPerDesc);
  qInfo() << "[QtAliceVision] MFeatures clear";
}

} // namespace qtAliceVision


#include "MFeatures.moc"