#pragma once

#include "FloatTexture.hpp"
#include "Surface.hpp"
#include "ShaderImageViewer.hpp"

#include <QQuickItem>
#include <QUrl>
#include <QRunnable>
#include <QSGGeometryNode>
#include <QSGSimpleMaterial>

#include <QSharedPointer>
#include <QVariant>
#include <QVector4D>

#include <memory>
#include <string>


namespace qtAliceVision
{

/**
    * @brief QRunnable object dedicated to load image using AliceVision.
    */
class FloatImageIORunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:

    explicit FloatImageIORunnable(const QUrl& path, int downscaleLevel = 0, QObject* parent = nullptr);

    /// Load image at path
    Q_SLOT void run() override;

    /// Emitted when the image is loaded
    Q_SIGNAL void resultReady(QSharedPointer<qtAliceVision::FloatImage> image, QSize sourceSize, const QVariantMap& metadata);

private:
    QUrl _path;
    int _downscaleLevel;
};

/**
    * @brief Load and display image.
    */
class FloatImageViewer : public QQuickItem
{
    Q_OBJECT
        /// Path to image
        Q_PROPERTY(QUrl source MEMBER _source NOTIFY sourceChanged)
        /// 
        Q_PROPERTY(float gamma MEMBER _gamma NOTIFY gammaChanged)
        /// 
        Q_PROPERTY(float gain MEMBER _gain NOTIFY gainChanged)
        ///
        Q_PROPERTY(QSize textureSize MEMBER _textureSize NOTIFY textureSizeChanged)
        ///
        Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)
        /// Whether the image is currently being loaded from file
        Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
        /// Whether to clear image between two loadings
        Q_PROPERTY(bool clearBeforeLoad MEMBER _clearBeforeLoad NOTIFY clearBeforeLoadChanged)

        Q_PROPERTY(EChannelMode channelMode MEMBER _channelMode NOTIFY channelModeChanged)

        Q_PROPERTY(QVariantMap metadata READ metadata NOTIFY metadataChanged)

        Q_PROPERTY(QList<QPoint> vertices READ vertices NOTIFY verticesChanged)     // --> Surface

        Q_PROPERTY(int downscaleLevel READ getDownscaleLevel WRITE setDownscaleLevel NOTIFY downscaleLevelChanged)

        Q_PROPERTY(Surface* surface READ getSurfacePtr NOTIFY surfaceChanged)

public:
    explicit FloatImageViewer(QQuickItem* parent = nullptr);
    ~FloatImageViewer() override;

    bool loading() const
    {
        return _loading;
    }

    void setLoading(bool loading);

    QSize sourceSize() const
    {
        return _sourceSize;
    }

    const QVariantMap& metadata() const
    {
        return _metadata;
    }

    QList<QPoint> vertices() const     // --> Surface
    { 
        return _surface.vertices(); 
    }

    int getDownscaleLevel() const { return _downscaleLevel; }
    void setDownscaleLevel(int level)
    {
        if (level != _downscaleLevel)
        {
            // Level [0;3]
            if (level < 0 && level > 3) level = 2;

            qWarning() << "Set Downscale " << level;
            _downscaleLevel = level;

            reload();
            _imageChanged = true;
            Q_EMIT verticesChanged(true);
            Q_EMIT downscaleLevelChanged();
        }
    }

    enum class EChannelMode : quint8 { RGBA, RGB, R, G, B, A };
    Q_ENUM(EChannelMode)

public:
    Q_SIGNAL void sourceChanged();
    Q_SIGNAL void loadingChanged();
    Q_SIGNAL void clearBeforeLoadChanged();
    Q_SIGNAL void gammaChanged();
    Q_SIGNAL void gainChanged();
    Q_SIGNAL void textureSizeChanged();
    Q_SIGNAL void sourceSizeChanged();
    Q_SIGNAL void channelModeChanged();
    Q_SIGNAL void imageChanged();
    Q_SIGNAL void metadataChanged();
    Q_SIGNAL void verticesChanged(bool reinit);    // --> Surface
    Q_SIGNAL void sfmChanged();
    Q_SIGNAL void downscaleLevelChanged();
    Q_SIGNAL void surfaceChanged();

    Q_INVOKABLE QVector4D pixelValueAt(int x, int y);
    Q_INVOKABLE QPoint getVertex(int index);    // --> Surface
    Q_INVOKABLE QPoint getPrincipalPoint();    // --> Surface

    Q_INVOKABLE void setVertex(int index, float x, float y);    // --> Surface
    Q_INVOKABLE void displayGrid(bool display);    // --> Surface
    Q_INVOKABLE void defaultControlPoints();    // --> Surface
    Q_INVOKABLE void resized();    // --> Surface (Check if use ?)
    Q_INVOKABLE bool reinit();     // --> Surface (Check if use ?)
    Q_INVOKABLE void hasDistortion(bool distortion);    // --> Surface (Change Name : setDistoViewerEnabled())
    Q_INVOKABLE void updateSubdivisions(int subs);      // --> Surface
    Q_INVOKABLE void setSfmPath(const QString& path);   // --> Surface
    Q_INVOKABLE void setIdView(int id);                 // --> Surface
    Q_INVOKABLE void setPanoViewerEnabled(bool state);  // --> Surface
    Q_INVOKABLE void rotatePanoramaRadians(float yawRadians, float pitchRadians);   // --> Surface
    Q_INVOKABLE void rotatePanoramaDegrees(float yawDegrees, float pitchDegrees);   // --> Surface
    Q_INVOKABLE void mouseOver(bool state);     // --> Surface
    //Q_INVOKABLE void setDownscale(int level); // SLOT ?
    Q_INVOKABLE double getPitch();  // --> Surface
    Q_INVOKABLE double getYaw();    // --> Surface
    Q_INVOKABLE bool isMouseInside(float mx, float my); // --> Surface

    Surface* getSurfacePtr() { return &_surface; }

private:
    /// Reload image from source
    void reload();
    /// Handle result from asynchronous file loading
    Q_SLOT void onResultReady(QSharedPointer<qtAliceVision::FloatImage> image, QSize sourceSize, const QVariantMap& metadata);
    /// Custom QSGNode update
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;

private:
    void updatePaintSurface(QSGGeometryNode* root, QSGSimpleMaterial<ShaderData>* material, QSGGeometry* geometryLine, bool updateSfmData);

    QUrl _source;
    float _gamma = 1.f;
    float _gain = 1.f;

    bool _loading = false;
    bool _outdated = false;
    bool _clearBeforeLoad = true;

    bool _imageChanged = false;
    EChannelMode _channelMode;
    QSharedPointer<FloatImage> _image;
    QRectF _boundingRect;
    QSize _textureSize;
    QSize _sourceSize = QSize(0, 0);

    QVariantMap _metadata;

    Surface _surface;
    bool _createRoot = true;

    // Level of downscale for images of a Panorama
    int _downscaleLevel = 0;
};

}  // ns qtAliceVision

Q_DECLARE_METATYPE(qtAliceVision::FloatImage);
Q_DECLARE_METATYPE(QSharedPointer<qtAliceVision::FloatImage>);
