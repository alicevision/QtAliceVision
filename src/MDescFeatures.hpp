#pragma once

#include "MFeature.hpp"

#include <QObject>
#include <QQmlListProperty>
#include <QRunnable>
#include <QUrl>

namespace qtAliceVision 
{

/**
* @brief Contains features data relative to one specific describer type.
*/
class MDescFeatures : public QObject
{
    Q_OBJECT
        /// String list of all the describerTypes used. Values comes from QML.
        Q_PROPERTY(QString describerType MEMBER _describerType NOTIFY describerTypeChanged)
        /// The list of features
        Q_PROPERTY(QQmlListProperty<qtAliceVision::MFeature> features READ features NOTIFY featuresChanged)
        /// Whether features are currently being loaded from file
        Q_PROPERTY(bool loadingFeatures READ loadingFeatures NOTIFY loadingFeaturesChanged)
        /// Number of Landmarks
        Q_PROPERTY(int nbLandmarks MEMBER _nbLandmarks NOTIFY nbLandmarksChanged)
        /// Number of Tracks
        Q_PROPERTY(int nbTracks MEMBER _nbTracks NOTIFY nbTracksChanged)

public:
    explicit MDescFeatures(const QString& desc, const QUrl& folder, aliceVision::IndexT viewId, QObject* parent = nullptr);
    ~MDescFeatures() override;

    /// Return a QQmlListProperty based on the QList of MFeature.
    QQmlListProperty<MFeature> features() { return { this, _features }; }

    /// Return the number of landmarks corresponding to this describer.
    inline int getNbLandmarks() const { return _nbLandmarks; }
    /// Set the number of landmarks corresponding to this describer.
    inline void setNbLandmarks(unsigned int nb) { _nbLandmarks = nb; }

    /// Return the number of tracks corresponding to this describer.
    inline int getNbTracks() const { return _nbTracks; }
    /// Set the number of tracks corresponding to this describer.
    inline void setNbTracks(unsigned int nb) { _nbTracks = nb; }

    /// Return a pointer to the QList of MFeature.
    inline const QList<MFeature*>& getFeaturesData() const { return _features; }
    /// Set the pointer to the QList of MFeature.
    void setFeaturesData(const QList<MFeature*>& features)
    {
        if (_features == features) return;

        qDeleteAll(_features); // Free memory of the current features
        _features.clear(); // Clear the list
        _features = features;
    }

    /// Return if features are currently loading or not.
    inline bool loadingFeatures() const { return _loadingFeatures; }
    /// Set if features are currently loading or not.
    void setLoadingFeatures(bool loadingFeatures)
    {
        if (_loadingFeatures == loadingFeatures)
            return;
        _loadingFeatures = loadingFeatures;
        Q_EMIT loadingFeaturesChanged(); // Useful because it will make QML aware of the loading status
    }

    /// Return if features are currently ready to be globally updated and displayed.
    inline bool ready() const { return _ready;  }
    /// Set if features are currently ready to be globally updated and displayed.
    void setReady(bool ready) { _ready = ready;  }

    /// Set a new feature folder path.
    void setFeatureFolder(const QUrl& folder)
    {
        if (_featureFolder == folder)
            return;
        _featureFolder = folder;
        reloadFeatures();
    }

    /// Set a new view id.
    void setViewId(aliceVision::IndexT id)
    {
        if (_viewId == id)
            return;
        _viewId = id;
        reloadFeatures();
    }

public:
    Q_SIGNAL void describerTypeChanged();
    Q_SIGNAL void featuresReadyToDisplayChanged(); // Used to tell the FeaturesWiewer (if there is one) to call the update function
    Q_SIGNAL void featuresChanged();
    Q_SIGNAL void loadingFeaturesChanged();
    Q_SIGNAL void nbLandmarksChanged();
    Q_SIGNAL void nbTracksChanged();
    Q_SIGNAL void updateDisplayTracks(bool boolean);
    Q_SIGNAL void featuresReadyChanged(const QString& desc); // Used to call the updateFeatures slot on the MFeatures instance

private:
    /// Clean the current features and create a thread to calculate the new ones.
    void reloadFeatures();
    /// Handle result from asynchronous file.
    void onFeaturesResultReady(QList<MFeature*> features);

    bool _loadingFeatures = false;
    bool _outdatedFeatures = false;
    bool _ready = false;

    QUrl _featureFolder;
    aliceVision::IndexT _viewId = aliceVision::UndefinedIndexT;
    QString _describerType = "sift";
    QList<MFeature*> _features;

    int _nbLandmarks = 0; // number of features associated to a 3D landmark
    int _nbTracks = 0; // number of tracks unvalidated after resection
};

} // namespace
