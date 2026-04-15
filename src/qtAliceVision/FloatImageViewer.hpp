#pragma once

#include "FloatTexture.hpp"
#include "SequenceCache.hpp"
#include "SingleImageLoader.hpp"
#include "Surface.hpp"

#include <QQuickItem>
#include <QSGNode>
#include <QVariant>
#include <QVector4D>

#include <memory>
#include <algorithm>

namespace qtAliceVision {

/**
 * @brief QQuickItem that loads and displays a floating-point image.
 *
 * Supports single-image loading and sequence playback, adjustable gamma/gain,
 * per-channel display modes, optional fisheye cropping, and panorama downscaling.
 *
 * The item integrates with Qt Quick's scene graph via a custom QSGNode and
 * exposes its state through QML-bindable properties and signals.
 */
class FloatImageViewer : public QQuickItem
{
    Q_OBJECT

    // -------------------------------------------------------------------------
    // Image source & loading
    // -------------------------------------------------------------------------

    /** @brief URL of the image file to display. */
    Q_PROPERTY(QUrl source MEMBER _source NOTIFY sourceChanged)

    /** @brief Current loading status. @see EStatus */
    Q_PROPERTY(EStatus status READ status NOTIFY statusChanged)

    /** @brief When true, clears the current image before starting a new load. */
    Q_PROPERTY(bool clearBeforeLoad MEMBER _clearBeforeLoad NOTIFY clearBeforeLoadChanged)

    // -------------------------------------------------------------------------
    // Display parameters
    // -------------------------------------------------------------------------

    /** @brief Gamma correction factor applied to the displayed image (default: 1.0). */
    Q_PROPERTY(float gamma MEMBER _gamma NOTIFY gammaChanged)

    /** @brief Gain (exposure multiplier) applied to the displayed image (default: 1.0). */
    Q_PROPERTY(float gain MEMBER _gain NOTIFY gainChanged)

    /** @brief Selects which channel(s) to display. @see EChannelMode */
    Q_PROPERTY(EChannelMode channelMode MEMBER _channelMode NOTIFY channelModeChanged)

    // -------------------------------------------------------------------------
    // Size & geometry
    // -------------------------------------------------------------------------

    /** @brief Size of the GPU texture used for rendering. */
    Q_PROPERTY(QSize textureSize MEMBER _textureSize NOTIFY textureSizeChanged)

    /** @brief Original pixel dimensions of the loaded image (read-only). */
    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)

    /** @brief Downscale level applied when loading panorama images (0 = full resolution). */
    Q_PROPERTY(int downscaleLevel READ getDownscaleLevel WRITE setDownscaleLevel NOTIFY downscaleLevelChanged)

    /** @brief Scale ratio of the viewer relative to the image size (clamped internally). */
    Q_PROPERTY(double resizeRatio READ getResizeRatio WRITE setResizeRatio NOTIFY resizeRatioChanged)

    // -------------------------------------------------------------------------
    // Surface & interaction
    // -------------------------------------------------------------------------

    /** @brief Pointer to the internal Surface object (read-only from QML). */
    Q_PROPERTY(Surface* surface READ getSurfacePtr NOTIFY surfaceChanged)

    /** @brief When true, pixel hover information is enabled for this viewer. */
    Q_PROPERTY(bool canBeHovered MEMBER _canBeHovered NOTIFY canBeHoveredChanged)

    // -------------------------------------------------------------------------
    // Fisheye
    // -------------------------------------------------------------------------

    /** @brief When true, the fisheye circle region is cropped from the display. */
    Q_PROPERTY(bool cropFisheye READ getCropFisheye WRITE setCropFisheye NOTIFY cropFisheyeChanged)

    // -------------------------------------------------------------------------
    // Metadata
    // -------------------------------------------------------------------------

    /** @brief Key/value map of image metadata extracted during loading. */
    Q_PROPERTY(QVariantMap metadata READ metadata NOTIFY metadataChanged)

    // -------------------------------------------------------------------------
    // Sequence & cache
    // -------------------------------------------------------------------------

    /** @brief Ordered list of file paths forming the image sequence. */
    Q_PROPERTY(QVariantList sequence READ getSequence WRITE setSequence NOTIFY sequenceChanged)

    /** @brief When true, sequence mode is active (as opposed to single-image mode). */
    Q_PROPERTY(bool useSequence MEMBER _useSequence NOTIFY useSequenceChanged)

    /** @brief When true, the sequence cache is actively prefetching frames. */
    Q_PROPERTY(bool fetchingSequence READ getFetchingSequence WRITE setFetchingSequence NOTIFY fetchingSequenceChanged)

    /** @brief List of frame indices currently held in the sequence cache. */
    Q_PROPERTY(QVariantList cachedFrames READ getCachedFrames NOTIFY cachedFramesChanged)

    /** @brief Maximum RAM (in GB) the sequence cache is allowed to consume. */
    Q_PROPERTY(int memoryLimit READ getMemoryLimit WRITE setMemoryLimit NOTIFY memoryLimitChanged)

    /**
     * @brief Current RAM usage of the sequence cache.
     * @note Re-evaluated whenever the cache changes; @c cachedFramesChanged is reused intentionally.
     * @return QPointF where x = used GB, y = total allocated GB.
     */
    Q_PROPERTY(QPointF ramInfo READ getRamInfo NOTIFY cachedFramesChanged)

  public:
    explicit FloatImageViewer(QQuickItem* parent = nullptr);
    ~FloatImageViewer() override;

    // -------------------------------------------------------------------------
    // Loading status
    // -------------------------------------------------------------------------

    /**
     * @brief Describes the current state of the image loading pipeline.
     */
    enum class EStatus : quint8
    {
        NONE,              /**< Idle — no error detected. */
        LOADING,           /**< An image is currently being loaded. */
        OUTDATED_LOADING,  /**< A new load was requested while the previous one was still running. */
        MISSING_FILE,      /**< The requested file does not exist on disk. */
        LOADING_ERROR      /**< A generic loading error occurred. */
    };
    Q_ENUM(EStatus)

    /** @brief Returns the current loading status. */
    EStatus status() const { return _status; }

    /**
     * @brief Sets the loading status and emits statusChanged().
     * @param status New status value.
     */
    void setStatus(EStatus status);

    /** @brief Returns true while an asynchronous image load is in progress. */
    bool loading() const { return _loading; }

    /**
     * @brief Updates the loading flag and emits the relevant signals.
     * @param loading True if a load is in progress.
     */
    void setLoading(bool loading);

    // -------------------------------------------------------------------------
    // Size accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Returns the original pixel dimensions of the loaded image.
     * @return Image size in pixels, or QSize(0,0) when no image is loaded.
     */
    QSize sourceSize() const { return _sourceSize; }

    // -------------------------------------------------------------------------
    // Downscale
    // -------------------------------------------------------------------------

    /** @brief Returns the current downscale level. */
    int getDownscaleLevel() const { return _downscaleLevel; }

    /**
     * @brief Sets the downscale level, clamped to >= 0.
     * @param level Desired downscale level (0 = full resolution). Negative values are clamped to 0.
     * @note Emits downscaleLevelChanged() when the value changes.
     */
    void setDownscaleLevel(int level)
    {
        if (level == _downscaleLevel)
            return;
        _downscaleLevel = std::max(0, level);
        Q_EMIT downscaleLevelChanged();
    }

    // -------------------------------------------------------------------------
    // Channel mode
    // -------------------------------------------------------------------------

    /**
     * @brief Controls which image channel(s) are rendered.
     */
    enum class EChannelMode : quint8
    {
        RGBA, /**< All four channels composited normally. */
        RGB,  /**< Colour channels only; alpha channel is ignored. */
        R,    /**< Red channel displayed as greyscale. */
        G,    /**< Green channel displayed as greyscale. */
        B,    /**< Blue channel displayed as greyscale. */
        A     /**< Alpha channel displayed as greyscale. */
    };
    Q_ENUM(EChannelMode)

    // -------------------------------------------------------------------------
    // Fisheye
    // -------------------------------------------------------------------------

    /** @brief Returns true if the fisheye crop is active. */
    bool getCropFisheye() const { return _cropFisheye; }

    /**
     * @brief Enables or disables the fisheye circle crop.
     * @param cropFisheye True to crop the fisheye region from the display.
     * @note Emits cropFisheyeChanged() when the value changes.
     */
    void setCropFisheye(bool cropFisheye)
    {
        if (_cropFisheye == cropFisheye)
        {
            return;
        }

        _cropFisheye = cropFisheye;
        Q_EMIT cropFisheyeChanged();
    }

    // -------------------------------------------------------------------------
    // Metadata
    // -------------------------------------------------------------------------

    /**
     * @brief Returns the image metadata map extracted during loading.
     * @return Read-only reference to the key/value metadata map.
     */
    const QVariantMap& metadata() const { return _metadata; }

    // -------------------------------------------------------------------------
    // Surface
    // -------------------------------------------------------------------------

    /**
     * @brief Returns a pointer to the internal Surface object.
     * @return Non-owning pointer to the Surface; lifetime is tied to this viewer.
     */
    Surface* getSurfacePtr() { return &_surface; }

    // -------------------------------------------------------------------------
    // Sequence & cache controls
    // -------------------------------------------------------------------------

    /**
     * @brief Returns the current image sequence as a list of file path strings.
     * @return Copy of the sequence path list.
     */
    QVariantList getSequence() const { return _sequence; }

    /**
     * @brief Sets the image sequence from an ordered list of file paths.
     * @param paths List of QUrl or QString entries representing the sequence frames.
     */
    void setSequence(const QVariantList& paths);

    /**
     * @brief Sets the resize ratio applied to the viewer (clamped internally).
     * @param ratio Desired scale ratio.
     */
    void setResizeRatio(double ratio);

    /**
     * @brief Returns the current (clamped) resize ratio.
     * @return Scale ratio value.
     */
    double getResizeRatio();

    /**
     * @brief Enables or disables active sequence prefetching.
     * @param fetching True to start prefetching; false to stop.
     */
    void setFetchingSequence(bool fetching);

    /**
     * @brief Returns true if the sequence cache is actively prefetching frames.
     * @return True while prefetching is in progress.
     */
    bool getFetchingSequence();

    /**
     * @brief Sets the maximum amount of RAM (in GB) the sequence cache may use.
     * @param memoryLimit Memory limit in gigabytes.
     */
    void setMemoryLimit(int memoryLimit);

    /**
     * @brief Returns the configured memory limit for the sequence cache.
     * @return Memory limit in gigabytes.
     */
    int getMemoryLimit();

    /**
     * @brief Returns the list of frame indices currently held in the sequence cache.
     * @return List of cached frame indices.
     */
    QVariantList getCachedFrames() const;

    /**
     * @brief Returns the current RAM usage of the sequence cache.
     * @return QPointF where x = used GB and y = total allocated GB.
     */
    QPointF getRamInfo() const;

    // -------------------------------------------------------------------------
    // Signals
    // -------------------------------------------------------------------------

    // Source & loading
    Q_SIGNAL void sourceChanged();
    Q_SIGNAL void statusChanged();
    Q_SIGNAL void clearBeforeLoadChanged();

    // Display parameters
    Q_SIGNAL void gammaChanged();
    Q_SIGNAL void gainChanged();
    Q_SIGNAL void channelModeChanged();

    // Size & geometry
    Q_SIGNAL void textureSizeChanged();
    Q_SIGNAL void sourceSizeChanged();
    Q_SIGNAL void downscaleLevelChanged();
    Q_SIGNAL void resizeRatioChanged();
    Q_SIGNAL void targetSizeChanged();

    // Surface & interaction
    Q_SIGNAL void surfaceChanged();
    Q_SIGNAL void canBeHoveredChanged();

    // Fisheye
    Q_SIGNAL void cropFisheyeChanged();
    Q_SIGNAL void fisheyeCircleParametersChanged();

    // Metadata & image
    Q_SIGNAL void imageChanged();
    Q_SIGNAL void metadataChanged();

    // Sequence & cache
    Q_SIGNAL void sequenceChanged();
    Q_SIGNAL void useSequenceChanged();
    Q_SIGNAL void fetchingSequenceChanged();
    Q_SIGNAL void cachedFramesChanged();
    Q_SIGNAL void memoryLimitChanged();

    // -------------------------------------------------------------------------
    // Invokables
    // -------------------------------------------------------------------------

    /**
     * @brief Returns the RGBA pixel value at the given image-space coordinates.
     * @param x Horizontal pixel coordinate (image space).
     * @param y Vertical pixel coordinate (image space).
     * @return Floating-point RGBA value as a QVector4D(r, g, b, a).
     */
    Q_INVOKABLE QVector4D pixelValueAt(int x, int y);

    /**
     * @brief Starts or stops sequence playback.
     * @param active True to begin playback; false to pause.
     */
    Q_INVOKABLE void playback(bool active);

  protected:
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

  private:
    /**
     * @brief Triggers a reload of the image from the current source URL.
     *
     * Called whenever the source, downscale level, or other load-affecting
     * properties change.
     */
    void reload();

    /**
     * @brief Provides the custom QSG scene-graph node used for GPU rendering.
     * @param oldNode Previously returned node, or nullptr on first call.
     * @param data Additional paint node data provided by Qt Quick.
     * @return Updated or newly created QSGNode.
     */
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;

    // --- Source & loading state ---
    QUrl _source;
    EStatus _status = EStatus::NONE;
    bool _loading = false;
    bool _outdated = false;
    bool _clearBeforeLoad = true;

    // --- Display parameters ---
    float _gamma = 1.f;
    bool _gammaChanged = false;
    float _gain = 1.f;
    bool _gainChanged = false;
    EChannelMode _channelMode = EChannelMode::RGBA;
    bool _channelModeChanged = false;

    // --- Geometry & size ---
    QRectF _boundingRect;
    QSize _textureSize;
    QSize _sourceSize = QSize(0, 0);
    bool _geometryChanged = false;
    double _clampedResizeRatio = 1.0;  /**< @brief Clamped resize ratio; exposed to QML as the @c resizeRatio property. */

    // --- Image data ---
    std::shared_ptr<FloatImage> _image;
    bool _imageChanged = false;
    QVariantMap _metadata;

    // --- Surface ---
    Surface _surface;
    bool _createRoot = true;  /**< @brief Prevents updating the surface before the root item is created. */

    // --- Downscale ---
    int _downscaleLevel = 0;  /**< @brief Downscale level for panorama images (0 = full resolution). */

    // --- Interaction ---
    bool _canBeHovered = false;
    bool _mouseOverChanged = false;

    // --- Fisheye ---
    bool _cropFisheye = false;

    // --- Sequence & cache ---
    imgserve::SequenceCache _sequenceCache;
    imgserve::SingleImageLoader _singleImageLoader;
    QVariantList _sequence;
    bool _useSequence = true;
    bool _fetchingSequence = false;
};

}  // namespace qtAliceVision

Q_DECLARE_METATYPE(qtAliceVision::FloatImage)
Q_DECLARE_METATYPE(std::shared_ptr<qtAliceVision::FloatImage>)
