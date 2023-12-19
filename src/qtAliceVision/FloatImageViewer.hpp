#pragma once

#include "FloatTexture.hpp"
#include "Surface.hpp"
#include "ShaderImageViewer.hpp"
#include "SequenceCache.hpp"
#include "SingleImageLoader.hpp"

#include <aliceVision/image/all.hpp>

#include <QQuickItem>
#include <QRunnable>
#include <QSGGeometryNode>
#include <QSGSimpleMaterial>
#include <QVariant>
#include <QVector4D>
#include <QList>

#include <memory>
#include <string>
#include <algorithm>

namespace qtAliceVision {

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

    Q_PROPERTY(EStatus status READ status NOTIFY statusChanged)

    Q_PROPERTY(bool clearBeforeLoad MEMBER _clearBeforeLoad NOTIFY clearBeforeLoadChanged)

    Q_PROPERTY(EChannelMode channelMode MEMBER _channelMode NOTIFY channelModeChanged)

    Q_PROPERTY(QVariantMap metadata READ metadata NOTIFY metadataChanged)

    Q_PROPERTY(int downscaleLevel READ getDownscaleLevel WRITE setDownscaleLevel NOTIFY downscaleLevelChanged)

    Q_PROPERTY(Surface* surface READ getSurfacePtr NOTIFY surfaceChanged)

    Q_PROPERTY(bool cropFisheye READ getCropFisheye WRITE setCropFisheye NOTIFY isCropFisheyeChanged)

    Q_PROPERTY(QVariantList sequence WRITE setSequence NOTIFY sequenceChanged)

    Q_PROPERTY(int targetSize WRITE setTargetSize NOTIFY targetSizeChanged)

    Q_PROPERTY(QVariantList cachedFrames READ getCachedFrames NOTIFY cachedFramesChanged)

    Q_PROPERTY(bool useSequence MEMBER _useSequence NOTIFY useSequenceChanged)

  public:
    explicit FloatImageViewer(QQuickItem* parent = nullptr);
    ~FloatImageViewer() override;

    enum class EStatus : quint8
    {
        NONE,              // nothing is happening, no error has been detected
        LOADING,           // an image is being loaded
        OUTDATED_LOADING,  // an image was already loading during the previous reload()
        MISSING_FILE,      // the file to load is missing
        ERROR              // generic error
    };
    Q_ENUM(EStatus)

    EStatus status() const { return _status; }

    void setStatus(EStatus status);

    bool loading() const { return _loading; }

    void setLoading(bool loading);

    QSize sourceSize() const { return _sourceSize; }

    const QVariantMap& metadata() const { return _metadata; }

    int getDownscaleLevel() const { return _downscaleLevel; }
    void setDownscaleLevel(int level)
    {
        if (level == _downscaleLevel)
            return;

        _downscaleLevel = std::max(0, level);
        Q_EMIT downscaleLevelChanged();
    }

    enum class EChannelMode : quint8
    {
        RGBA,
        RGB,
        R,
        G,
        B,
        A
    };
    Q_ENUM(EChannelMode)

    bool getCropFisheye() const { return _cropFisheye; }
    void setCropFisheye(bool cropFisheye) { _cropFisheye = cropFisheye; }

    Q_SIGNAL void isCropFisheyeChanged();

    // Q_SIGNALS
    Q_SIGNAL void sourceChanged();
    Q_SIGNAL void statusChanged();
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
    Q_SIGNAL void sfmRequiredChanged();
    Q_SIGNAL void fisheyeCircleParametersChanged();
    Q_SIGNAL void sequenceChanged();
    Q_SIGNAL void targetSizeChanged();
    Q_SIGNAL void cachedFramesChanged();
    Q_SIGNAL void useSequenceChanged();

    // Q_INVOKABLE
    Q_INVOKABLE QVector4D pixelValueAt(int x, int y);
    Q_INVOKABLE void playback(bool active);

    Surface* getSurfacePtr() { return &_surface; }

    void setSequence(const QVariantList& paths);

    void setTargetSize(int size);

    QVariantList getCachedFrames() const;

  private:
    /// Reload image from source
    void reload();

    /// Custom QSGNode update
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;

    void updatePaintSurface(QSGGeometryNode* root, QSGSimpleMaterial<ShaderData>* material, QSGGeometry* geometryLine);

    QUrl _source;
    float _gamma = 1.f;
    float _gain = 1.f;

    EStatus _status = EStatus::NONE;
    bool _loading = false;
    bool _outdated = false;
    bool _clearBeforeLoad = true;

    bool _imageChanged = false;
    EChannelMode _channelMode;
    std::shared_ptr<FloatImage> _image;
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

    imgserve::SequenceCache _sequenceCache;
    imgserve::SingleImageLoader _singleImageLoader;
    bool _useSequence = true;
};

}  // namespace qtAliceVision

Q_DECLARE_METATYPE(qtAliceVision::FloatImage)
Q_DECLARE_METATYPE(std::shared_ptr<qtAliceVision::FloatImage>)
