#pragma once

#include <MFeatures.hpp>
#include <MSfMData.hpp>
#include <MTracks.hpp>
#include <Painter.hpp>

#include <QQuickItem>
#include <QSGGeometry>

namespace qtAliceVision {

  /**
   * @brief Display extracted features / Matches / Tracks
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

      /// Data properties

      // Describer type
      Q_PROPERTY(QString describerType MEMBER _describerType NOTIFY describerTypeChanged)
      // Pointer to Features
      Q_PROPERTY(qtAliceVision::MFeatures* mfeatures READ getMFeatures WRITE setMFeatures NOTIFY featuresChanged)

  public:
    /// Helpers

    enum FeatureDisplayMode {
      Points = 0,         // Simple points (GL_POINTS)
      Squares,            // Scaled filled squares (GL_TRIANGLES)
      OrientedSquares     // Scaled and oriented squares (GL_LINES)
    };
    Q_ENUM(FeatureDisplayMode)

    enum TrackDisplayMode {
      LinesOnly = 0,
      WithCurrentMatches,
      WithAllMatches
    };
    Q_ENUM(TrackDisplayMode)

    struct PaintParams {
      bool haveValidFeatures = false;
      bool haveValidTracks = false;
      bool haveValidLandmarks = false;

      float minFeatureScale = std::numeric_limits<float>::min();
      float maxFeatureScale = std::numeric_limits<float>::max();

      int nbFeaturesToDraw = 0;
      int nbMatchesToDraw = 0;
      int nbLandmarksToDraw = 0;
    };

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

    Q_SIGNAL void describerTypeChanged();
    Q_SIGNAL void featuresChanged();

    /// Public methods

    explicit FeaturesViewer(QQuickItem* parent = nullptr);
    ~FeaturesViewer() override;

    MFeatures* getMFeatures() { return _mfeatures; }
    void setMFeatures(MFeatures* sfmData);

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
    bool _displayTracks = true;
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

    QString _describerType = "sift";
    MFeatures* _mfeatures = nullptr;

    Painter painter = Painter({
      "features",
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
      "landmarks"
    });
  };

} // namespace qtAliceVision
