#pragma once

#include <MFeature.hpp>
#include <MSfMData.hpp>
#include <MTracks.hpp>

#include <aliceVision/sfm/pipeline/regionsIO.hpp>
#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/track/Track.hpp>
#include <aliceVision/sfm/sfmStatistics.hpp>

#include <QQuickItem>
#include <QRunnable>
#include <QUrl>


namespace qtAliceVision {


/**
 * @brief Load and display extracted features of a View.
 */
class FeaturesViewer : public QQuickItem
{
    Q_OBJECT
    /// Path to folder containing the features
    Q_PROPERTY(QUrl featureFolder MEMBER _featureFolder NOTIFY featureFolderChanged)
    /// ViewID to consider
    Q_PROPERTY(quint32 viewId MEMBER _viewId NOTIFY viewIdChanged)
    /// Describer type to load
    Q_PROPERTY(QString describerType MEMBER _describerType NOTIFY describerTypeChanged)
    /// Pointer to SfmData
    Q_PROPERTY(qtAliceVision::MSfMData* msfmData READ getMSfmData WRITE setMSfmData NOTIFY sfmDataChanged)
    /// Pointer to Tracks
    Q_PROPERTY(qtAliceVision::MTracks* mtracks READ getMTracks WRITE setMTracks NOTIFY tracksChanged)
    /// Display mode (see DisplayMode enum)
    Q_PROPERTY(DisplayMode displayMode MEMBER _displayMode NOTIFY displayModeChanged)
    /// Features color
    Q_PROPERTY(QColor color MEMBER _color NOTIFY colorChanged)
    /// Landmarks color
    Q_PROPERTY(QColor landmarkColor MEMBER _landmarkColor NOTIFY landmarkColorChanged)
    /// Whether features are currently being loaded from file
    Q_PROPERTY(bool loadingFeatures READ loadingFeatures NOTIFY loadingFeaturesChanged)
    /// Display all the 2D features extracted from the image
    Q_PROPERTY(bool displayfeatures MEMBER _displayFeatures NOTIFY displayFeaturesChanged)
    /// Display the 3D reprojection of the features associated to a landmark
    Q_PROPERTY(bool displayLandmarks MEMBER _displayLandmarks NOTIFY displayLandmarksChanged)
    /// Display the center of the tracks unvalidated after resection
    Q_PROPERTY(bool displayTracks MEMBER _displayTracks NOTIFY displayFeaturesChanged)
    /// Number of features reconstructed
    Q_PROPERTY(int nbLandmarks MEMBER _nbLandmarks NOTIFY displayLandmarksChanged)
    /// Number of tracks
    Q_PROPERTY(int nbTracks MEMBER _nbTracks NOTIFY displayTracksChanged)
    /// The list of features
    Q_PROPERTY(QQmlListProperty<qtAliceVision::MFeature> features READ features NOTIFY featuresChanged)

public:
    enum DisplayMode {
        Points = 0,         // Simple points (GL_POINTS)
        Squares,            // Scaled filled squares (GL_TRIANGLES)
        OrientedSquares     // Scaled and oriented squares (GL_LINES)
    };
    Q_ENUM(DisplayMode)

    explicit FeaturesViewer(QQuickItem* parent = nullptr);
    ~FeaturesViewer() override;

    QQmlListProperty<MFeature> features()
    {
        return {this, _features};
    }

    inline bool loadingFeatures() const { return _loadingFeatures; }
    inline void setLoadingFeatures(bool loadingFeatures)
    {
        if(_loadingFeatures == loadingFeatures)
            return;
        _loadingFeatures = loadingFeatures;
        Q_EMIT loadingFeaturesChanged();
    }

    MSfMData* getMSfmData() { return _msfmData; }
    void setMSfmData(MSfMData* sfmData);

    MTracks* getMTracks() { return _mtracks; }
    void setMTracks(MTracks* tracks);

public:
    Q_SIGNAL void sfmDataChanged();
    Q_SIGNAL void featureFolderChanged();
    Q_SIGNAL void viewIdChanged();
    Q_SIGNAL void describerTypeChanged();
    Q_SIGNAL void featuresChanged();
    Q_SIGNAL void loadingFeaturesChanged();
    Q_SIGNAL void tracksChanged();

    Q_SIGNAL void displayFeaturesChanged();
    Q_SIGNAL void displayLandmarksChanged();
    Q_SIGNAL void displayTracksChanged();

    Q_SIGNAL void displayModeChanged();
    Q_SIGNAL void colorChanged();
    Q_SIGNAL void landmarkColorChanged();

private:
    /// Custom QSGNode update
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;

private:
    /// Reload features from source
    void reloadFeatures();
    /// Handle result from asynchronous file loading
    Q_SLOT void onFeaturesResultReady(QList<MFeature*> features);

    void reloadTracks();
    void clearTracksFromFeatures();
    void updateFeatureFromTracks();
    void updateFeatureFromTracksEmit();
    /**
     * @brief Update the number of tracks not associated to a landmark.
     * @warning depends on features, landmarks (sfmData), and tracks.
     */
    void updateNbTracks();

    void clearSfMFromFeatures();
    void updateFeatureFromSfM();
    void updateFeatureFromSfMEmit();

    void updatePaintFeatures(QSGNode* oldNode, QSGNode* node);
    void updatePaintTracks(QSGNode* oldNode, QSGNode* node);
    void updatePaintLandmarks(QSGNode* oldNode, QSGNode* node);

    QUrl _featureFolder;
    aliceVision::IndexT _viewId = aliceVision::UndefinedIndexT;
    QString _describerType = "sift";
    QList<MFeature*> _features;
    MSfMData* _msfmData = nullptr;
    MTracks* _mtracks = nullptr;

    int _nbLandmarks = 0; //< number of features associated to a 3D landmark
    int _nbTracks = 0; // number of tracks unvalidated after resection
    DisplayMode _displayMode = FeaturesViewer::Points;
    QColor _color = QColor(20, 220, 80);
    QColor _landmarkColor = QColor(255, 0, 0);

    bool _loadingFeatures = false;
    bool _outdatedFeatures = false;

    bool _displayFeatures = true;
    bool _displayLandmarks = true;
    bool _displayTracks = true;
};

} // namespace

Q_DECLARE_METATYPE(qtAliceVision::MFeature);   // for usage in signals/slots
