#pragma once

#include <QQuickItem>
#include <QRunnable>
#include <QUrl>


#include <aliceVision/sfm/pipeline/regionsIO.hpp>


namespace qtAliceVision
{

/**
 * @brief QObject wrapper around a SIOPointFeature.
 */
class Feature : public QObject
{
    Q_OBJECT

    Q_PROPERTY(float x READ x CONSTANT)
    Q_PROPERTY(float y READ y CONSTANT)
    Q_PROPERTY(float scale READ scale CONSTANT)
    Q_PROPERTY(float orientation READ orientation CONSTANT)

public:
    Feature() {};
    Feature(const Feature& other) { _feat = other._feat; }

    Feature(const aliceVision::feature::SIOPointFeature& feat, QObject* parent=nullptr):
    QObject(parent),
    _feat(feat)
    {}

    inline float x() const { return _feat.x(); }
    inline float y() const { return _feat.y(); }
    inline float scale() const { return _feat.scale(); }
    inline float orientation() const { return _feat.orientation(); }

    const aliceVision::feature::SIOPointFeature& pointFeature() const { return _feat; }

private:
    aliceVision::feature::SIOPointFeature _feat;
};



/**
 * @brief QRunnable object dedicated to load features using AliceVision.
 */
class FeatureIORunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:    
    /// io parameters: folder, viewId, describerType
    using IOParams = std::tuple<QUrl, aliceVision::IndexT, QString>;

    FeatureIORunnable(const IOParams& params):
    _params(params)
    {}

    /// Load features based on input parameters
    Q_SLOT void run() override;

    /**
     * @brief  Emitted when features have been loaded and Features objects created.
     * @warning Features objects are not parented - their deletion must be handled manually.
     * 
     * @param features the loaded Features list
     */    
    Q_SIGNAL void resultReady(QList<Feature*> features);
    
private:
    IOParams _params;
};

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
    /// Display mode (see DisplayMode enum)
    Q_PROPERTY(DisplayMode displayMode MEMBER _displayMode NOTIFY displayModeChanged)
    /// Features main color
    Q_PROPERTY(QColor color MEMBER _color NOTIFY colorChanged)
    /// Whether features are currently being laoded from file
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    /// Whether to clear features between two loadings
    Q_PROPERTY(bool clearBeforeLoad MEMBER _clearBeforeLoad NOTIFY clearBeforeLoadChanged)
    /// The list of features
    Q_PROPERTY(QQmlListProperty<qtAliceVision::Feature> features READ features NOTIFY featuresChanged)
   
public:
    enum DisplayMode {
        Points = 0,         // Simple points (GL_POINTS)
        Squares,            // Scaled filled squares (GL_TRIANGLES)
        OrientedSquares     // Scaled and oriented squares (GL_LINES)
    };
    Q_ENUM(DisplayMode)

    FeaturesViewer(QQuickItem* parent = nullptr);
    ~FeaturesViewer();

    QQmlListProperty<Feature> features() { 
        return QQmlListProperty<Feature>(this, _features);
    }

    inline bool loading() const { return _loading; }
    inline void setLoading(bool loading) { 
        if(_loading == loading) 
            return;
        _loading = loading;
        Q_EMIT loadingChanged();
    }

public:
    Q_SIGNAL void folderChanged();
    Q_SIGNAL void viewIdChanged();
    Q_SIGNAL void describerTypeChanged();
    Q_SIGNAL void featuresChanged();
    Q_SIGNAL void loadingChanged();
    Q_SIGNAL void clearBeforeLoadChanged();
    Q_SIGNAL void displayModeChanged();
    Q_SIGNAL void colorChanged();

protected:
    /// Reload features from source
    void reload();
    /// Handle result from asynchronous file loading
    Q_SLOT void onResultReady(QList<Feature*> features);
    /// Custom QSGNode update
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;

private:
    QUrl _folder;
    aliceVision::IndexT _viewId = aliceVision::UndefinedIndexT;
    QString _describerType = "sift";
    QList<Feature*> _features;
    DisplayMode _displayMode = FeaturesViewer::Points;
    QColor _color = QColor(20, 220, 80);

    bool _loading = false;
    bool _outdated = false;
    bool _clearBeforeLoad = true;
};

} // namespace

Q_DECLARE_METATYPE(qtAliceVision::Feature);   // for usage in signals/slots
