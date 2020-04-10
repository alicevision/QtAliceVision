#pragma once

#include <QQuickItem>
#include <QRunnable>
#include <QUrl>


#include <aliceVision/sfm/pipeline/regionsIO.hpp>
#include <aliceVision/sfmData/SfMData.hpp>
#include <MFeature.hpp>
#include <MSfMData.hpp>

namespace qtAliceVision
{


/**
 * @brief Load and display extracted features of a View.
 */
class FeaturesViewer : public QQuickItem
{
    Q_OBJECT
    /// Path to folder containing the features
    Q_PROPERTY(QUrl folder MEMBER _folder NOTIFY folderChanged)
    /// ViewID to consider
    Q_PROPERTY(quint32 viewId MEMBER _viewId NOTIFY viewIdChanged)
    /// Describer type to load
    Q_PROPERTY(QString describerType MEMBER _describerType NOTIFY describerTypeChanged)
    /// Pointer to sfmData
    Q_PROPERTY(qtAliceVision::MSfMData* msfmData READ getMSfmData WRITE setMSfmData NOTIFY sfmDataChanged)
    /// Display mode (see DisplayMode enum)
    Q_PROPERTY(DisplayMode displayMode MEMBER _displayMode NOTIFY displayModeChanged)
    /// Features color
    Q_PROPERTY(QColor color MEMBER _color NOTIFY colorChanged)
    /// Landmarks color
    Q_PROPERTY(QColor landmarkColor MEMBER _landmarkColor NOTIFY landmarkColorChanged)
    /// Whether features are currently being loaded from file
    Q_PROPERTY(bool loadingFeatures READ loadingFeatures NOTIFY loadingFeaturesChanged)
    /// Whether to clear features between two loadings
    Q_PROPERTY(bool clearFeaturesBeforeLoad MEMBER _clearFeaturesBeforeLoad NOTIFY clearFeaturesBeforeLoadChanged)
    /// Display all the 2D features extracted from the image
    Q_PROPERTY(bool displayfeatures MEMBER _displayFeatures NOTIFY displayFeaturesChanged)
    /// Display the 3D reprojection of the features associated to a landmark
    Q_PROPERTY(bool displayLandmarks MEMBER _displayLandmarks NOTIFY displayLandmarksChanged)
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

    QQmlListProperty<MFeature> features() {
        return {this, _features};
    }

    inline bool loadingFeatures() const { return _loadingFeatures; }
    inline void setLoadingFeatures(bool loadingFeatures) { 
        if(_loadingFeatures == loadingFeatures) 
            return;
        _loadingFeatures = loadingFeatures;
        Q_EMIT loadingFeaturesChanged();
    }

    MSfMData* getMSfmData() { return _msfmData; }
    void setMSfmData(MSfMData* sfmData);

public:
    Q_SIGNAL void sfmDataChanged();
    Q_SIGNAL void folderChanged();
    Q_SIGNAL void viewIdChanged();
    Q_SIGNAL void describerTypeChanged();
    Q_SIGNAL void featuresChanged();
    Q_SIGNAL void loadingFeaturesChanged();
    Q_SIGNAL void clearFeaturesBeforeLoadChanged();

    Q_SIGNAL void displayFeaturesChanged();
    Q_SIGNAL void displayLandmarksChanged();

    Q_SIGNAL void displayModeChanged();
    Q_SIGNAL void colorChanged();
    Q_SIGNAL void landmarkColorChanged();

protected:
    /// Reload features from source
    void reloadFeatures();
    /// Handle result from asynchronous file loading
    Q_SLOT void onFeaturesResultReady(QList<MFeature*> features);

    /// Custom QSGNode update
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;

private:
    void clearSfMFromFeatures();
    void updateFeatureFromSfM();

    void updatePaintFeatures(QSGNode* oldNode, QSGNode* node);
    void updatePaintLandmarks(QSGNode* oldNode, QSGNode* node);

    QUrl _folder;
    aliceVision::IndexT _viewId = aliceVision::UndefinedIndexT;
    QString _describerType = "sift";
    QList<MFeature*> _features;
    MSfMData* _msfmData = nullptr;
    int _nbLandmarks = 0; //< number of features associated to a 3D landmark
    DisplayMode _displayMode = FeaturesViewer::Points;
    QColor _color = QColor(20, 220, 80);
    QColor _landmarkColor = QColor(255, 0, 0);

    bool _loadingFeatures = false;
    bool _outdatedFeatures = false;
    bool _clearFeaturesBeforeLoad = true;

    bool _displayFeatures = true;
    bool _displayLandmarks = true;
};

} // namespace

Q_DECLARE_METATYPE(qtAliceVision::MFeature);   // for usage in signals/slots
