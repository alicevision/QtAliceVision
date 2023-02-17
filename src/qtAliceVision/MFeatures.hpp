#pragma once

#include "MFeature.hpp"
#include "MSfMData.hpp"
#include "MTracks.hpp"

#include <aliceVision/types.hpp>

#include <QObject>
#include <QRunnable>
#include <QUrl>
#include <QVariantMap>

#include <map>

namespace qtAliceVision
{

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
    Q_PROPERTY(bool enableTimeWindow MEMBER _enableTimeWindow NOTIFY enableTimeWindowChanged)
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
    /// Status Enum

    enum Status
    {
        None = 0,
        Loading,
        Ready,
        Error
    };
    Q_ENUM(Status)

    /// Helpers

    struct MViewFeatures
    {
        QList<MFeature*> features;

        aliceVision::IndexT frameId = aliceVision::UndefinedIndexT;
        float maxFeatureScale = std::numeric_limits<float>::min();
        float minFeatureScale = std::numeric_limits<float>::max();
        int nbLandmarks = 0; // number of features of the view associated to a 3D landmark
        int nbTracks = 0;    // number of tracks unvalidated after resection
    };
    using MViewFeaturesPerView = std::map<aliceVision::IndexT, MViewFeatures>;
    using MViewFeaturesPerViewPerDesc = std::map<QString, MViewFeaturesPerView>;

    struct MTrackFeatures
    {
        std::map<aliceVision::IndexT, MFeature*> featuresPerFrame;

        aliceVision::IndexT minFrameId = std::numeric_limits<aliceVision::IndexT>::max();
        aliceVision::IndexT maxFrameId = std::numeric_limits<aliceVision::IndexT>::min();
        int nbLandmarks = 0;             // number of features of the track associated to a 3D landmark
        float featureScaleAverage = 0.f; // average feature scale
    };
    using MTrackFeaturesPerTrack = std::map<aliceVision::IndexT, MTrackFeatures>;
    using MTrackFeaturesPerTrackPerDesc = std::map<QString, MTrackFeaturesPerTrack>;

    enum class ReconstructionState
    {
        None = 0,
        Partial = 1,
        Complete = 2
    };

    struct MGlobalTrackInfo
    {
        int nbFeatures = 0;
        int nbLandmarks = 0;
        aliceVision::IndexT startFrameId = std::numeric_limits<aliceVision::IndexT>::max();
        aliceVision::IndexT endFrameId = std::numeric_limits<aliceVision::IndexT>::min();

        ReconstructionState reconstructionState() const
        {
            if (nbLandmarks == 0)
                return ReconstructionState::None;
            if (nbLandmarks < nbFeatures)
                return ReconstructionState::Partial;
            return ReconstructionState::Complete;
        }
    };
    using MGlobalTrackInfoPerTrack = std::map<aliceVision::IndexT, MGlobalTrackInfo>;

    /// Slots

    Q_SLOT void load();
    Q_SLOT void clearAndLoad();
    Q_SLOT void onFeaturesReady(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc);
    Q_SLOT void loadGlobalInfo();
    Q_SLOT void updateGlobalTrackInfo(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc);

    /// Signals

    Q_SIGNAL void featureFolderChanged();
    Q_SIGNAL void describerTypesChanged();
    Q_SIGNAL void currentViewIdChanged();
    Q_SIGNAL void enableTimeWindowChanged();
    Q_SIGNAL void timeWindowChanged();
    Q_SIGNAL void featuresInfoChanged();

    Q_SIGNAL void featuresChanged();
    Q_SIGNAL void tracksChanged();
    Q_SIGNAL void sfmDataChanged();

    Q_SIGNAL void statusChanged(Status status);

    /// Public methods

    MFeatures();
    MFeatures(const MFeatures& other) = default;
    ~MFeatures() override;

    MTracks* getMTracks() { return _mtracks; }
    void setMTracks(MTracks* tracks);

    MSfMData* getMSfmData() { return _msfmData; }
    void setMSfmData(MSfMData* sfmData);

    /**
     * @brief Allows to know if feature info can be accessed / are available
     * @return true if the feature info can be accessed
     */
    bool haveValidFeatures() const { return (!_viewFeaturesPerViewPerDesc.empty()) && (_status == Ready); }

    /**
     * @brief Allows to know if SfMData info can be accessed / are available
     * @return true if the SfMData info can be accessed
     */
    bool haveValidLandmarks() const
    {
        return (_msfmData != nullptr) && (_msfmData->status() == MSfMData::Ready) &&
               (_msfmData->rawDataPtr() != nullptr);
    }

    /**
     * @brief Allows to know if track info can be accessed / are available
     * @return true if the track info can be accessed
     */
    bool haveValidTracks() const
    {
        return (_mtracks != nullptr) && (_mtracks->status() == MTracks::Ready) && (_mtracks->tracksPtr() != nullptr);
    }

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
     * @brief Get the minimum feature scale for a given decriber type
     * @param[in] describerType The given describer type
     * @return minimum feature scale (or std::numeric_limits<float>::min())
     */
    float getMinFeatureScale(const QString& describerType) const;

    /**
     * @brief Get the maximum feature scale for a given decriber type
     * @param[in] describerType The given describer type
     * @return maximum feature scale (or std::numeric_limits<float>::max())
     */
    float getMaxFeatureScale(const QString& describerType) const;

    /**
     * @brief Get current view feature info for a given decriber type
     * @param[in] describerType The given describer type
     * @return MViewFeatures pointer (or nullptr)
     */
    const MViewFeatures* getCurrentViewFeatures(const QString& describerType) const;

    /**
     * @brief Build / Organize feature info per track for a given decriber type
     * @param[in] describerType The given describer type
     * @return MTrackFeaturesPerTrack pointer (or nullptr)
     */
    const MTrackFeaturesPerTrack* getTrackFeaturesPerTrack(const QString& describerType) const;

    /**
     * @brief Allow access of feature info directly in QML
     * @return QVariantMap<describerType, QMap<viewId, QMap<info, value>>>
     */
    const QVariantMap& featuresInfo() const { return _featuresInfo; }

    /**
     * @brief Provides access to global track information (i.e not only limited to the current time window)
     * @param trackId the ID of the track
     * @return struct containing global information on the track
     */
    MGlobalTrackInfo& globalTrackInfo(aliceVision::IndexT trackId) { return _globalTrackInfoPerTrack[trackId]; }

    /**
     * @brief Get MFeatures status
     * @see MFeatures status enum
     * @return MFeatures status enum
     */
    Status status() const { return _status; }

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
    /// Private methods (to use with Loading status)

    /**
     * @brief Get the list of all viewIds in the current SfM data
     * @param[in,out] viewIds The list of viewIds to load in memory
     */
    void getAllViewIds(std::vector<aliceVision::IndexT>& viewIds);

    /**
     * @brief Get the list of view ids to load in memory
     * Handle muliple views (time window) / single view cases.
     * @note Implements simple caching mechanism to avoid loading view information already in memory, removed unused
     * view information.
     * @param[in,out] viewIds The list of viewIds to load in memory
     */
    void getViewIdsToLoad(std::vector<aliceVision::IndexT>& viewIds);

    /**
     * @brief Update MViewFeatures information with Tracks information
     * @return true if MViewFeatures information have been updated
     */
    bool updateFromTracks(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc);

    /**
     * @brief Update MViewFeatures information with SfMData information
     * @return true if MViewFeatures information have been updated
     */
    bool updateFromSfM(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc);

    /**
     * @brief Update min / max feature scale per describer
     */
    void updateMinMaxFeatureScale();

    /**
     * @brief Update / Compute per track information:
     *        - Build Per track feature access structure _trackFeaturesPerTrackPerDesc
     *        - Compute per track feature scale score
     *        - Compute track mathes reprojection error
     */
    void updatePerTrackInformation();

    /**
     * @brief Update the QVariantMap featuresInfo (useful for QML) from loaded data
     */
    void updateUIFeaturesInformation();

    void clearViewFeaturesPerViewPerDesc(MViewFeaturesPerViewPerDesc* viewFeaturesPerViewPerDesc);
    void clearAllTrackInfo();
    void clearAllSfMInfo();
    void clearAllFeatureReprojection();
    void clearAll();

    void setLocalInfoStatus(Status status);
    void setGlobalInfoStatus(Status status);

    /// Private members

    // inputs
    QUrl _featureFolder;
    QVariantList _describerTypes;
    aliceVision::IndexT _currentViewId;

    bool _outdatedFeatures = false; // set to true if a loading request occurs during another
    bool _enableTimeWindow = false; // set to true if the user request multiple frames (e.g. display tracks)
    int _timeWindow = 1; // size of the time window (from current frame - time window to current frame + time window), 0
                         // is disable, -1 is no limit

    Status _localInfoStatus = MFeatures::None;
    Status _globalInfoStatus = MFeatures::Ready;

    // internal data
    std::map<QString, float> _minFeatureScalePerDesc;
    std::map<QString, float> _maxFeatureScalePerDesc;

    MViewFeaturesPerViewPerDesc _viewFeaturesPerViewPerDesc;
    MTrackFeaturesPerTrackPerDesc _trackFeaturesPerTrackPerDesc;
    QVariantMap _featuresInfo;
    MGlobalTrackInfoPerTrack _globalTrackInfoPerTrack;

    /// external data
    MSfMData* _msfmData = nullptr;
    MTracks* _mtracks = nullptr;

    /// status
    Status _status = MFeatures::None;
};

} // namespace qtAliceVision
