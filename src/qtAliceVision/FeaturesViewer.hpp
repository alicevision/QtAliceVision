#pragma once

#include <MFeatures.hpp>
#include <MSfMData.hpp>
#include <MTracks.hpp>
#include <Painter.hpp>

#include <QQuickItem>
#include <QSGGeometry>

#include <vector>
#include <unordered_map>
#include <string>

namespace qtAliceVision {

/**
 * @brief Cached reconstruction data used for drawing by the FeaturesViewer.
 *
 * Given a describer type, reconstruction data is organized in two parts:
 * - for each view, the features data with 3D reconstruction information if there is any
 * - the tracks, containing track elements ordered by frame number.
 */
class MReconstruction
{
  public:
    struct FeatureData
    {
        float x, y;
        float rx, ry;
        float scale;
        float orientation;
        bool hasTrack;
        bool hasLandmark;
    };

    struct PointwiseTrackData
    {
        aliceVision::IndexT frameId = aliceVision::UndefinedIndexT;
        aliceVision::IndexT viewId = aliceVision::UndefinedIndexT;
        aliceVision::IndexT featureId = aliceVision::UndefinedIndexT;
    };

    struct TrackData
    {
        std::vector<PointwiseTrackData> elements;
        float averageScale;
        int nbReconstructed;
    };

    std::unordered_map<aliceVision::IndexT, std::vector<FeatureData>> featureDatasPerView;
    std::vector<TrackData> trackDatas;
    float minFeatureScale;
    float maxFeatureScale;
};

/**
 * @brief Display extracted features, matches, tracks and landmarks in 2D.
 *
 * The FeaturesViewer uses MFeatures, MTracks and MSfMData to load data from disk
 * and keeps an MReconstruction instance to cache the reconstruction data
 * in a way that makes sense for drawing.
 *
 * The various display options and filters can be manipulated from QML.
 *
 * The FeaturesViewer relies on the Painter class for paiting routines and layer organization.
 */
class FeaturesViewer : public QQuickItem
{
    Q_OBJECT

    /// Display properties

    // Display all the 2D features extracted from the image
    Q_PROPERTY(bool displayFeatures MEMBER _displayFeatures NOTIFY displayFeaturesChanged)
    // Display the of the center of the tracks over time
    Q_PROPERTY(bool displayTracks MEMBER _displayTracks NOTIFY displayTracksChanged)
    // Display the center of the tracks unvalidated after resection
    Q_PROPERTY(bool displayMatches MEMBER _displayMatches NOTIFY displayMatchesChanged)
    // Display the 3D reprojection of the features associated to a landmark
    Q_PROPERTY(bool displayLandmarks MEMBER _displayLandmarks NOTIFY displayLandmarksChanged)
    // Feature display mode (see FeatureDisplayMode enum)
    Q_PROPERTY(FeatureDisplayMode featureDisplayMode MEMBER _featureDisplayMode NOTIFY featureDisplayModeChanged)
    // Track display mode (see TrackDisplayMode enum)
    Q_PROPERTY(TrackDisplayMode trackDisplayMode MEMBER _trackDisplayMode NOTIFY trackDisplayModeChanged)
    // Minimum feature scale to display
    Q_PROPERTY(float featureMinScaleFilter MEMBER _featureMinScaleFilter NOTIFY featureMinScaleFilterChanged)
    // Minimum feature scale to display
    Q_PROPERTY(float featureMaxScaleFilter MEMBER _featureMaxScaleFilter NOTIFY featureMaxScaleFilterChanged)
    // Display 3d tracks
    Q_PROPERTY(bool display3dTracks MEMBER _display3dTracks NOTIFY display3dTracksChanged)
    // Display only contiguous tracks
    Q_PROPERTY(bool trackContiguousFilter MEMBER _trackContiguousFilter NOTIFY trackContiguousFilterChanged)
    // Display only tracks with at least one inlier
    Q_PROPERTY(bool trackInliersFilter MEMBER _trackInliersFilter NOTIFY trackInliersFilterChanged)
    // Display track endpoints
    Q_PROPERTY(bool displayTrackEndpoints MEMBER _displayTrackEndpoints NOTIFY displayTrackEndpointsChanged)
    // Features color
    Q_PROPERTY(QColor featureColor MEMBER _featureColor NOTIFY featureColorChanged)
    // Matches color
    Q_PROPERTY(QColor matchColor MEMBER _matchColor NOTIFY matchColorChanged)
    // Landmarks color
    Q_PROPERTY(QColor landmarkColor MEMBER _landmarkColor NOTIFY landmarkColorChanged)
    // Currenty selected ViewId
    Q_PROPERTY(quint32 currentViewId MEMBER _currentViewId NOTIFY currentViewIdChanged)
    // Time window to consider
    Q_PROPERTY(bool enableTimeWindow MEMBER _enableTimeWindow NOTIFY enableTimeWindowChanged)
    Q_PROPERTY(int timeWindow MEMBER _timeWindow NOTIFY timeWindowChanged)

    /// Data properties

    // Describer type
    Q_PROPERTY(QString describerType MEMBER _describerType NOTIFY describerTypeChanged)
    // Pointer to Features
    Q_PROPERTY(qtAliceVision::MFeatures* mfeatures READ getMFeatures WRITE setMFeatures NOTIFY featuresChanged)
    // Pointer to Tracks
    Q_PROPERTY(qtAliceVision::MTracks* mtracks READ getMTracks WRITE setMTracks NOTIFY tracksChanged)
    // Pointer to SfmData
    Q_PROPERTY(qtAliceVision::MSfMData* msfmData READ getMSfMData WRITE setMSfMData NOTIFY sfmDataChanged)

  public:
    /// Helpers

    enum FeatureDisplayMode
    {
        Points = 0,      // Simple points (GL_POINTS)
        Squares,         // Scaled filled squares (GL_TRIANGLES)
        OrientedSquares  // Scaled and oriented squares (GL_LINES)
    };
    Q_ENUM(FeatureDisplayMode)

    enum TrackDisplayMode
    {
        LinesOnly = 0,
        WithCurrentMatches,
        WithAllMatches
    };
    Q_ENUM(TrackDisplayMode)

    struct PaintParams
    {
        bool haveValidFeatures = false;
        bool haveValidTracks = false;
        bool haveValidLandmarks = false;

        float minFeatureScale = std::numeric_limits<float>::min();
        float maxFeatureScale = std::numeric_limits<float>::max();
    };

    /// Slots

    Q_SLOT void updateReconstruction();

    /// Signals

    Q_SIGNAL void displayFeaturesChanged();
    Q_SIGNAL void displayTracksChanged();
    Q_SIGNAL void displayMatchesChanged();
    Q_SIGNAL void displayLandmarksChanged();

    Q_SIGNAL void featureDisplayModeChanged();
    Q_SIGNAL void trackDisplayModeChanged();

    Q_SIGNAL void featureMinScaleFilterChanged();
    Q_SIGNAL void featureMaxScaleFilterChanged();

    Q_SIGNAL void display3dTracksChanged();
    Q_SIGNAL void trackContiguousFilterChanged();
    Q_SIGNAL void trackInliersFilterChanged();
    Q_SIGNAL void displayTrackEndpointsChanged();

    Q_SIGNAL void featureColorChanged();
    Q_SIGNAL void matchColorChanged();
    Q_SIGNAL void landmarkColorChanged();

    Q_SIGNAL void currentViewIdChanged();
    Q_SIGNAL void enableTimeWindowChanged();
    Q_SIGNAL void timeWindowChanged();

    Q_SIGNAL void describerTypeChanged();
    Q_SIGNAL void featuresChanged();
    Q_SIGNAL void tracksChanged();
    Q_SIGNAL void sfmDataChanged();
    Q_SIGNAL void reconstructionChanged();

    /// Public methods

    explicit FeaturesViewer(QQuickItem* parent = nullptr);
    ~FeaturesViewer() override;

    MFeatures* getMFeatures() { return _mfeatures; }
    void setMFeatures(MFeatures* features);

    MTracks* getMTracks() { return _mtracks; }
    void setMTracks(MTracks* tracks);

    MSfMData* getMSfMData() { return _msfmdata; }
    void setMSfMData(MSfMData* sfmData);

  private:
    /// Custom QSGNode update
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;

  private:
    void initializePaintParams(PaintParams& params);
    void updatePaintFeatures(const PaintParams& params, QSGNode* node);
    void paintFeaturesAsPoints(const PaintParams& params, QSGNode* node);
    void paintFeaturesAsSquares(const PaintParams& params, QSGNode* node);
    void paintFeaturesAsOrientedSquares(const PaintParams& params, QSGNode* node);
    void updatePaintTracks(const PaintParams& params, QSGNode* node);
    void updatePaintMatches(const PaintParams& params, QSGNode* node);
    void updatePaintLandmarks(const PaintParams& params, QSGNode* node);

    bool _displayFeatures = true;
    bool _displayTracks = false;
    bool _displayMatches = true;
    bool _displayLandmarks = true;

    FeatureDisplayMode _featureDisplayMode = FeaturesViewer::Points;
    TrackDisplayMode _trackDisplayMode = FeaturesViewer::WithCurrentMatches;

    float _featureMinScaleFilter = 0.f;
    float _featureMaxScaleFilter = 1.f;

    bool _display3dTracks = false;
    bool _trackContiguousFilter = true;
    bool _trackInliersFilter = false;
    bool _displayTrackEndpoints = true;

    QColor _featureColor = QColor(20, 220, 80);
    QColor _matchColor = QColor(255, 127, 0);
    QColor _landmarkColor = QColor(255, 0, 0);
    QColor _endpointColor = QColor(80, 80, 80);

    aliceVision::IndexT _currentViewId;
    bool _enableTimeWindow = false;
    int _timeWindow = 1;

    QString _describerType = "sift";
    MFeatures* _mfeatures = nullptr;
    MTracks* _mtracks = nullptr;
    MSfMData* _msfmdata = nullptr;
    MReconstruction _mreconstruction;

    Painter painter = Painter({"features",
                               "trackEndpoints",
                               "highlightPoints",
                               "trackLines_reconstruction_none",
                               "trackLines_reconstruction_partial_outliers",
                               "trackLines_reconstruction_partial_inliers",
                               "trackLines_reconstruction_full",
                               "trackLines_gaps",
                               "trackPoints_outliers",
                               "trackPoints_inliers",
                               "matches",
                               "reprojectionErrors",
                               "landmarks"});
};

}  // namespace qtAliceVision
