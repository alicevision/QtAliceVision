#pragma once

#include "MFeature.hpp"
#include "MSfMdata.hpp"
#include "MTracks.hpp"
#include "MDescFeatures.hpp"

#include <QObject>
#include <QUrl>
#include <QSharedPointer>

namespace qtAliceVision 
{

/**
* @brief Contains all the data about the features. Contains a map of MDescFeatures.
*/
class MFeatures : public QObject
{
    Q_OBJECT
        /// String list of all the describerTypes used. Values comes from QML.
        Q_PROPERTY(QStringList describerTypes MEMBER _describerTypes NOTIFY describerTypesChanged)
        /// Path to folder containing the features
        Q_PROPERTY(QUrl featureFolder MEMBER _featureFolder NOTIFY featureFolderChanged)
        /// ViewID to consider
        Q_PROPERTY(quint32 viewId MEMBER _viewId NOTIFY viewIdChanged)
        /// Pointer to SfmData
        Q_PROPERTY(qtAliceVision::MSfMData* msfmData READ getMSfmData WRITE setMSfmData NOTIFY sfmDataChanged)
        /// Pointer to Tracks
        Q_PROPERTY(qtAliceVision::MTracks* mtracks READ getMTracks WRITE setMTracks NOTIFY tracksChanged)
        /// The list of features
        Q_PROPERTY(QVariantMap allFeatures READ allFeatures NOTIFY mapChanged)
        /// Whether features are currently being loaded from file
        Q_PROPERTY(bool loadingFeatures READ loadingFeatures NOTIFY loadingFeaturesChanged)
        /// Whether landmarks are reconstructed
        Q_PROPERTY(bool haveValidLandmarks READ haveValidLandmarks NOTIFY sfmDataChanged)
        /// Whether tracks are reconstructed
        Q_PROPERTY(bool haveValidTracks READ haveValidTracks NOTIFY tracksChanged)
        /// Number of all the landmarks
        Q_PROPERTY(unsigned int nbLandmarks READ getNbLandmarks)
        /// Number of all the tracks
        Q_PROPERTY(unsigned int nbTracks READ getNbTracks)

public:
    explicit MFeatures(QObject* parent = nullptr);
    ~MFeatures() override;

    /// Return a QVariantMap containing the MDescFeatures pointers.
    QVariantMap allFeatures() const
    {
        QVariantMap map;
        for (auto i = _allFeatures.constBegin(); i != _allFeatures.constEnd(); ++i)
        {
            map.insert(i.key(), QVariant::fromValue<MDescFeatures*>(i.value().get()));
        }
        return map;
    }

    /// Return the total number of landmarks.
    inline unsigned int getNbLandmarks() const { return _nbLandmarks; }
    /// Return the total number of tracks.
    inline unsigned int getNbTracks() const { return _nbTracks; }

    /// Return a pointer to SFM Data.
    inline MSfMData* getMSfmData() const { return _msfmData; }
    /// Set a pointer to SFM Data.
    void setMSfmData(MSfMData* sfmData);

    /// Return a pointer to Tracks Data.
    inline MTracks* getMTracks() const { return _mtracks; }
    /// Set a pointer to Tracks Data.
    void setMTracks(MTracks* tracks);

    /// Return a reference to the QMap containing the MDecFeatures pointers.
    inline const QMap<QString, QSharedPointer<MDescFeatures>>& getAllFeatures() const { return _allFeatures; }

    /// Return if global process is currently loading or not.
    inline bool loadingFeatures() const { return _loadingFeatures; }
    /// Return if global process is currently loading or not.
    void setLoadingFeatures(bool loadingFeatures)
    {
        if (_loadingFeatures == loadingFeatures) return;
        _loadingFeatures = loadingFeatures;
        Q_EMIT loadingFeaturesChanged();
    }

    /// Update the global loading features (used in QML). Based on each MDescFeatures own loading status.
    void updateLoadingFeatures()
    {
        _loadingFeatures = false;
        for (auto i = _allFeatures.constBegin(); i != _allFeatures.constEnd(); ++i)
        {
            if (i.value()->loadingFeatures())
                _loadingFeatures = true;
                break;
        }
        Q_EMIT loadingFeaturesChanged();
    }

    /// Check if every MDecFeatures is ready to be globally updated and displayed
    bool checkAllFeaturesReady() const
    {
        for (auto i = _allFeatures.constBegin(); i != _allFeatures.constEnd(); ++i)
        {
            if (!i.value()->ready()) return false;
        }
        return true;
    }
         
    /// Return if landmarks (SFM Data) are valid or not.
    bool haveValidLandmarks() const { return _msfmData != nullptr && _msfmData->status() == MSfMData::Ready; }
    /// Return if tracks are valid or not.
    bool haveValidTracks() const { return _mtracks != nullptr && _mtracks->status() == MTracks::Ready; }
 
public:
    Q_SIGNAL void sfmDataChanged();
    Q_SIGNAL void tracksChanged();
    Q_SIGNAL void featuresChanged();
    Q_SIGNAL void mapChanged();
    Q_SIGNAL void viewIdChanged();
    Q_SIGNAL void featureFolderChanged();
    Q_SIGNAL void describerTypesChanged();
    Q_SIGNAL void loadingFeaturesChanged();

private:
    /// Setup the basic connections between signals and slots.
    void setupBasicConnections();

    /// Reload the QMap and setup specific connections between signals and slots.
    void reloadQMap();
    /// Update feature folder to each MDescFeatures.
    void updateMapFeatureFolder();
    /// Update view id to each MDescFeatures.
    void updateMapViewId();

    /// Update all features.
    void updateFeatures(const QString& desc);
    /// Reload all features.
    void reloadFeatures();

    /// Clear tracks from every feature passed in argument.
    void clearTracksFromFeatures(const QList<MFeature*>& features);
    /// Update every feature described by the describer type passed in argument using tracks data.
    void updateFeatureFromTracks(const QString& desc);
    /// Update every feature using tracks data. Should be connected to a signal.
    void updateFeatureFromTracksEmit();

    /**
    * @brief Update the number of tracks not associated to a landmark. Update number of tracks per describer + total number.
    * @warning depends on features, landmarks (sfmData), and tracks.
    */
    void updateNbTracks();
    /// Update the total number of landmarks.
    void updateTotalNbLandmarks();

    /// Clear SFM data from every feature passed in argument.
    void clearSfMFromFeatures(const QList<MFeature*>& features);
    /// Update every feature described by the describer type passed in argument using SFM data.
    void updateFeatureFromSfM(const QString& desc);
    /// Update every feature using SFM data. Should be connected to a signal.
    void updateFeatureFromSfMEmit();


    QStringList _describerTypes;
    QUrl _featureFolder;
    aliceVision::IndexT _viewId = aliceVision::UndefinedIndexT;
    QMap<QString, QSharedPointer<MDescFeatures>> _allFeatures;
    MSfMData* _msfmData = nullptr;
    MTracks* _mtracks = nullptr;
    bool _loadingFeatures = false;

    int _nbLandmarks = 0; // Total number of features associated to a 3D landmark
    int _nbTracks = 0; // Total number of tracks unvalidated after resection
};

} // namespace
