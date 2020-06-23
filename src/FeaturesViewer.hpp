#pragma once

#include "MDescFeatures.hpp"

#include <QQuickItem>
#include <QSharedPointer>

namespace qtAliceVision
{

/**
 * @brief Load and display extracted features of a View.
 * @warning FeaturesViewer contains a pointer to an instance of MDescFeatures: it refers only to one type of decribers.
 */
class FeaturesViewer : public QQuickItem
{
    Q_OBJECT
    /// Display mode (see DisplayMode enum)
    Q_PROPERTY(DisplayMode displayMode MEMBER _displayMode NOTIFY displayModeChanged)
    /// Describer type represented by this viewer
    Q_PROPERTY(QString describerType MEMBER _describerType NOTIFY describerTypeChanged)
    /// Features color
    Q_PROPERTY(QColor color MEMBER _color NOTIFY colorChanged)
    /// Landmarks color
    Q_PROPERTY(QColor landmarkColor MEMBER _landmarkColor NOTIFY landmarkColorChanged)
    /// Display all the 2D features extracted from the image
    Q_PROPERTY(bool displayfeatures MEMBER _displayFeatures NOTIFY displayFeaturesChanged)
    /// Display the 3D reprojection of the features associated to a landmark
    Q_PROPERTY(bool displayLandmarks MEMBER _displayLandmarks NOTIFY displayLandmarksChanged)
    /// Display the center of the tracks unvalidated after resection
    Q_PROPERTY(bool displayTracks MEMBER _displayTracks NOTIFY displayTracksChanged)
    /// Number of features reconstructed
    Q_PROPERTY(int nbLandmarks READ getNbLandmarks NOTIFY displayLandmarksChanged)
    /// Number of tracks
    Q_PROPERTY(int nbTracks READ getNbTracks NOTIFY displayTracksChanged)
    /// MDescFeatures pointer
    Q_PROPERTY(qtAliceVision::MDescFeatures* mdescFeatures READ getMDescFeatures WRITE setMDescFeatures NOTIFY descFeaturesChanged)


public:
    enum DisplayMode {
        Points = 0,         // Simple points (GL_POINTS)
        Squares,            // Scaled filled squares (GL_TRIANGLES)
        OrientedSquares     // Scaled and oriented squares (GL_LINES)
    };
    Q_ENUM(DisplayMode)

    explicit FeaturesViewer(QQuickItem* parent = nullptr);
    ~FeaturesViewer() override = default;

    /// Get the number of tracks corresponding to the describer type.
    inline int getNbTracks() const { return _mdescFeatures->getNbTracks(); }
    /// Get the number of landmarks corresponding to the describer type.
    inline int getNbLandmarks() const { return _mdescFeatures->getNbLandmarks(); }

    /// Return a pointer to the MDescFeatures instance.
    MDescFeatures* getMDescFeatures() const { return _mdescFeatures; }
    /// Set a MDescFeatures instance to the class member '_mdescFeatures'.
    void setMDescFeatures(MDescFeatures* descFeatures);

    /// Set the class member '_displayTracks' with a boolean.
    void setDisplayTracks(bool displayTracks) {
        if (_displayTracks == displayTracks)
            return;
        _displayTracks = displayTracks;
        Q_EMIT displayTracksChanged();
    }

public:
    Q_SIGNAL void displayFeaturesChanged();
    Q_SIGNAL void describerTypeChanged();
    Q_SIGNAL void displayLandmarksChanged();
    Q_SIGNAL void displayTracksChanged();
    Q_SIGNAL void descFeaturesChanged();
    Q_SIGNAL void displayModeChanged();
    Q_SIGNAL void colorChanged();
    Q_SIGNAL void landmarkColorChanged();

private:
    /// Custom QSGNode update
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;
    /// Update Features display using QSGNode.
    void updatePaintFeatures(QSGNode* oldNode, QSGNode* node);
    /// Update Tracks display using QSGNode.
    void updatePaintTracks(QSGNode* oldNode, QSGNode* node);
    /// Update Landmarks display using QSGNode.
    void updatePaintLandmarks(QSGNode* oldNode, QSGNode* node);

    DisplayMode _displayMode = FeaturesViewer::Points;
    QColor _color = QColor(20, 220, 80);
    QColor _landmarkColor = QColor(255, 0, 0);

    bool _loadingFeatures = false;
    bool _outdatedFeatures = false;

    bool _displayFeatures = true;
    bool _displayLandmarks = true;
    bool _displayTracks = true;

    MDescFeatures* _mdescFeatures = nullptr;
    QString _describerType = "sift";
};

} // namespace
