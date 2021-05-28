#pragma once

#include "MFeature.hpp"
#include "MTracks.hpp"
#include "MSfMData.hpp"

#include <aliceVision/types.hpp>

#include <QObject>
#include <QRunnable>
#include <QUrl>
#include <QVariantMap>

#include <map>

namespace qtAliceVision {

/**
 * @brief QObject that allows to manage / access view info & features
 */
class MFeatures : public QObject
{
    Q_OBJECT

    /// Data properties
    
    // Path to folder containing the features
    Q_PROPERTY(QUrl featureFolder MEMBER _featureFolder NOTIFY featureFolderChanged)
    // Describer types to load
    Q_PROPERTY(QVariantList describerTypes MEMBER _describerTypes NOTIFY describerTypesChanged)
    // ViewId to consider
    Q_PROPERTY(quint32 currentViewId MEMBER _currentViewId NOTIFY currentViewIdChanged)
    // Time window to consider
    Q_PROPERTY(bool loadTimeWindow MEMBER _loadTimeWindow NOTIFY loadTimeWindowChanged)
    // Time window to consider
    Q_PROPERTY(int timeWindow MEMBER _timeWindow NOTIFY timeWindowChanged)
    /// The list of features information (per view, per describer) for UI
    Q_PROPERTY(QVariantMap featuresInfo READ featuresInfo NOTIFY featuresInfoChanged)

    /// External data properties

    // Pointer to Tracks
    Q_PROPERTY(qtAliceVision::MTracks* mtracks READ getMTracks WRITE setMTracks NOTIFY tracksChanged)
    // Pointer to SfmData
    Q_PROPERTY(qtAliceVision::MSfMData* msfmData READ getMSfmData WRITE setMSfmData NOTIFY sfmDataChanged)
    // Whether landmarks are reconstructed
    Q_PROPERTY(bool haveValidLandmarks READ haveValidLandmarks NOTIFY sfmDataChanged)
    // Whether tracks are reconstructed
    Q_PROPERTY(bool haveValidTracks READ haveValidTracks NOTIFY tracksChanged)

    /// Status

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:

    // Status Enum

    enum Status {
      None = 0,
      Loading,
      Ready,
      Error
    };
    Q_ENUM(Status)

    // Helpers

    struct MViewFeatures {
      QList<MFeature*> features;

      aliceVision::IndexT frameId = aliceVision::UndefinedIndexT;
      int nbLandmarks = 0; // number of features associated to a 3D landmark
      int nbTracks = 0; // number of tracks unvalidated after resection
    };
    using MViewFeaturesPerView = std::map<aliceVision::IndexT, MViewFeatures>;
    using MViewFeaturesPerViewPerDesc = std::map<QString, MViewFeaturesPerView>;
    using MViewFeaturesPerFramePerTrack = std::map<int, std::map<aliceVision::IndexT, const MFeature*>>;

    // SLOTS

    Q_SLOT void load();
    Q_SLOT void onFeaturesReady(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc);

    // SIGNALS

    Q_SIGNAL void featureFolderChanged();
    Q_SIGNAL void describerTypesChanged();
    Q_SIGNAL void currentViewIdChanged();
    Q_SIGNAL void loadTimeWindowChanged();
    Q_SIGNAL void timeWindowChanged();
    Q_SIGNAL void featuresInfoChanged();

    Q_SIGNAL void featuresChanged();
    Q_SIGNAL void tracksChanged();
    Q_SIGNAL void sfmDataChanged();

    Q_SIGNAL void statusChanged(Status status);

    // Public methods

    MFeatures();
    MFeatures(const MFeatures& other) = default;
    ~MFeatures() override;

    MTracks* getMTracks() { return _mtracks; }
    void setMTracks(MTracks* tracks);

    MSfMData* getMSfmData()  { return _msfmData; }
    void setMSfmData(MSfMData* sfmData);

    /**
    * @brief Allows to know if feature info can be accessed / are available
    * @return true if the feature info can be accessed
    */
    bool haveValidFeatures() const { return (_viewFeaturesPerViewPerDesc != nullptr) && (_status == Ready); }

    /**
    * @brief Allows to know if SfMData info can be accessed / are available
    * @return true if the SfMData info can be accessed
    */
    bool haveValidLandmarks() const { return (_msfmData != nullptr) && (_msfmData->status() == MSfMData::Ready) && (_msfmData->rawDataPtr() != nullptr); }

    /**
    * @brief Allows to know if track info can be accessed / are available
    * @return true if the track info can be accessed
    */
    bool haveValidTracks() const { return (_mtracks != nullptr) && (_mtracks->status() == MTracks::Ready) && (_mtracks->tracksPtr() != nullptr); }

    /**
    * @brief Get time window value
    * MFeatures load feature info from current frame - time window to current frame + time window
    * @return time window value
    */
    int getTimeWindow() const { return _timeWindow; };

    /**
    * @brief Get the current view Id used by MFeatures
    * @return current view Id
    */
    aliceVision::IndexT getCurrentViewId() const { return _currentViewId; }

    /**
    * @brief Get the current frame Id used by MFeatures
    * @return current frame Id
    */
    aliceVision::IndexT getCurrentFrameId() const;

    /**
    * @brief Get current view feature info for a giver decriber type
    * @param[in] describerType The given describer type
    * @return MViewFeatures pointer (or nullptr)
    */
    const MViewFeatures* getCurrentViewFeatures(const QString& describerType) const;

    /**
     * @brief Build / Organize feature info per track for a giver decriber type
     * @param[in] describerType The given describer type
     * @param[in,out] viewFeaturesPerFramePerTrack
     */
    void getViewFeaturesPerFramePerTrack(const QString& describerType, MViewFeaturesPerFramePerTrack& viewFeaturesPerFramePerTrack) const;

    /**
     * @brief Allow access of feature info directly in QML
     * @return QVariantMap<describerType, QMap<viewId, QMap<info, value>>>
     */
    const QVariantMap& featuresInfo() const
    {
      return _featuresInfo;
    }

    /**
     * @brief Get MFeatures status 
     * @see MFeatures status enum
     * @return MFeatures status enum
     */
    Status status() const 
    { 
      return _status; 
    }

    void setStatus(Status status) 
    {
      if (status == _status)
        return;
      _status = status;

      Q_EMIT statusChanged(_status);

      if (status == Ready || status == Error)
        Q_EMIT featuresChanged();
    }

private:
    
    // Private methods (to use with Loading status)

    void updateFromTracks(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc);
    void updateFromSfM(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc);

    void updateFeaturesInfo();

    void clearViewFeaturesPerViewPerDesc(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc);
    void clearAllTrackInfo();
    void clearAllSfMInfo();
    void clearAll();
    
    // Private members

    QUrl _featureFolder;
    QVariantList _describerTypes;
    aliceVision::IndexT _currentViewId;
    int _timeWindow = 1;
    bool _loadTimeWindow = false;
    bool _outdatedFeatures = false;

    MViewFeaturesPerViewPerDesc* _viewFeaturesPerViewPerDesc = nullptr;

    MSfMData* _msfmData = nullptr;
    MTracks* _mtracks = nullptr;

    QVariantMap _featuresInfo;
    Status _status = MFeatures::None;
};

} // namespace qtAliceVision
