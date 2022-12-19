#pragma once

#include "FloatTexture.hpp"
#include "Surface.hpp"

#include <QQuickItem>
#include <QUrl>
#include <QRunnable>
#include <QSGGeometryNode>

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
    // Q_PROPERTIES
    Q_PROPERTY(QUrl source MEMBER _source NOTIFY sourceChanged)

    Q_PROPERTY(float gamma MEMBER _gamma NOTIFY gammaChanged)

    Q_PROPERTY(float gain MEMBER _gain NOTIFY gainChanged)

    Q_PROPERTY(bool canBeHovered MEMBER _canBeHovered NOTIFY canBeHoveredChanged)

    Q_PROPERTY(QSize textureSize MEMBER _textureSize NOTIFY textureSizeChanged)

    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)
    
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

    Q_PROPERTY(bool clearBeforeLoad MEMBER _clearBeforeLoad NOTIFY clearBeforeLoadChanged)
    
    Q_PROPERTY(EChannelMode channelMode MEMBER _channelMode NOTIFY channelModeChanged)
    
    Q_PROPERTY(QVariantMap metadata READ metadata NOTIFY metadataChanged)
   
    Q_PROPERTY(int downscaleLevel READ getDownscaleLevel WRITE setDownscaleLevel NOTIFY downscaleLevelChanged)
    
    Q_PROPERTY(Surface* surface READ getSurfacePtr NOTIFY surfaceChanged)

    Q_PROPERTY(bool cropFisheye READ getCropFisheye WRITE setCropFisheye NOTIFY isCropFisheyeChanged)

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

    int getDownscaleLevel() const { return _downscaleLevel; }
    void setDownscaleLevel(int level)
    {
        if (level == _downscaleLevel) return;

        // Level [0;3]
        if (level < 0 && level > 6) level = 4;
        _downscaleLevel = level;
        reload();
        //Q_EMIT downscaleLevelChanged();
    }

    enum class EChannelMode : quint8 { RGBA, RGB, R, G, B, A };
    Q_ENUM(EChannelMode)

    bool getCropFisheye() const { return _cropFisheye; }
    void setCropFisheye(bool cropFisheye) {
        _cropFisheye = cropFisheye;
    }

    Q_SIGNAL void isCropFisheyeChanged();

    //Q_SIGNALS
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
    Q_SIGNAL void downscaleLevelChanged();
    Q_SIGNAL void surfaceChanged();
    Q_SIGNAL void canBeHoveredChanged();
    Q_SIGNAL void fisheyeCircleParametersChanged();

    // Q_INVOKABLE
    Q_INVOKABLE QVector4D pixelValueAt(int x, int y);
    
    Surface* getSurfacePtr() { return &_surface; }

protected:
    virtual void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    /// Reload image from source
    void reload();
    /// Handle result from asynchronous file loading
    Q_SLOT void onResultReady(QSharedPointer<qtAliceVision::FloatImage> image, QSize sourceSize, const QVariantMap& metadata);
    /// Custom QSGNode update
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;

    QUrl _source;
    float _gamma = 1.f;
    bool _gammaChanged = false;
    float _gain = 1.f;
    bool _gainChanged = false;

    bool _loading = false;
    bool _outdated = false;
    bool _clearBeforeLoad = true;

    bool _geometryChanged = false;
    bool _imageChanged = false;
    EChannelMode _channelMode;
    bool _channelModeChanged = false;
    QSharedPointer<FloatImage> _image;
    QRectF _boundingRect;
    QSize _textureSize;
    QSize _sourceSize = QSize(0, 0);

    QVariantMap _metadata;

    Surface _surface;
    // Prevent to update surface without the root created
    bool _createRoot = true;
    // Level of downscale for images of a Panorama
    int _downscaleLevel = 0;

    bool _canBeHovered = false;

    bool _cropFisheye = false;
};

}

Q_DECLARE_METATYPE(qtAliceVision::FloatImage);
Q_DECLARE_METATYPE(QSharedPointer<qtAliceVision::FloatImage>);
