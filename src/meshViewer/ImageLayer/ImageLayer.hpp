#pragma once

#include <Core/LayerItem.hpp>
#include <SfmDataLayer/SfmDataObject.hpp>

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/pixelTypes.hpp>

#include <QFutureWatcher>
#include <QString>
#include <memory>

/** @brief Loaded image together with its distortion lookup map. */
struct ImageAndMap
{
    aliceVision::image::Image<aliceVision::image::RGBAfColor> image; /**< Image pixels, in sRGB. */
    /**
     * @brief Distortion lookup map, same size as the image.
     *
     * Each texel stores, in its red and green channels, the normalized coordinates of the distorted
     * pixel to sample for the corresponding undistorted pixel. Empty when the map was not rebuilt.
     */
    aliceVision::image::Image<aliceVision::image::RGBAfColor> map;
};

/** @brief Output of the background image loading task. */
struct ImageLoadResult
{
    ImageAndMap content;  /**< Loaded image and map. */
    QString errorString;  /**< Error message when loading failed. */
    bool valid = false;   /**< True when the image was loaded successfully. */
};

/**
 * @brief Layer that asynchronously loads an image file and draws it undistorted behind the scene.
 *
 * The image is loaded on a worker thread. When intrinsics are set with setIntrinsics(), an
 * undistortion lookup map is computed alongside the image so the renderer can remove the lens
 * distortion and align the image with the scene seen through an AVCameraInfo.
 */
class ImageLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Path or file:// URL of the image to load. */
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    /** @brief Error message of the last failed load, empty on success. */
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    /** @brief True while an image is being loaded. */
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    
  public:
    /**
     * @brief Build an empty layer.
     * @param[in] parent Optional Qt parent object.
     */
    explicit ImageLayer(QObject* parent = nullptr);
    /** @brief Wait for any pending load, then destroy the layer. */
    ~ImageLayer() override;

    /** @brief Get the image path. */
    QString source() const
    {
        return _source;
    }

    /**
     * @brief Start loading a new image in the background.
     * @param[in] path Path or file:// URL of the image. An empty path is ignored.
     * @note When the distortion map must be rebuilt, intrinsics must have been set with setIntrinsics().
     */
    void setSource(const QString& path);

    /** @brief Returns true while an image is being loaded. */
    bool loading() const
    {
        return _loading;
    }

    /** @brief Get the error message of the last failed load. */
    QString errorString() const
    {
        return _errorString;
    }

    /** @brief Returns true when a loaded image has not been taken by the renderable yet. */
    bool imageDirty() const
    {
        return _imageDirty;
    }

    /**
     * @brief Hand the loaded image over to the renderable and clear the dirty flag.
     * @return The image and map; the layer no longer holds them.
     */
    ImageAndMap takeImage();

    /**
     * @brief Use the intrinsics of a view to build the undistortion map on the next load.
     * @param[in] object SfM data holding the view.
     * @param[in] viewId Identifier of the view.
     * @note Must be called before changing source; ignored if the view has no intrinsics or they did not change.
     */
    Q_INVOKABLE void setIntrinsics(SfmDataObject* object, aliceVision::IndexT viewId);

  signals:
    /** @brief Emitted when the source path changes. */
    void sourceChanged();
    /** @brief Emitted when loading starts or ends. */
    void loadingChanged();
    /** @brief Emitted after each load completes. */
    void errorStringChanged();

  public:
    /** @brief Create an ImageRenderable. */
    std::unique_ptr<IRenderable> createRenderable() const override;

  private slots:
    /** @brief Collect the background load result, keeping the previous map when it was not rebuilt. */
    void onLoadFinished();

  private:
    /** @brief Image path. */
    QString _source;
    /** @brief Loading flag. */
    bool _loading = false;
    /** @brief Error message of the last failed load. */
    QString _errorString;
    /** @brief Intrinsics used to build the undistortion map. */
    std::shared_ptr<aliceVision::camera::IntrinsicBase> _intrinsics;
    /** @brief True when a loaded image waits for the renderable. */
    bool _imageDirty = false;
    /** @brief True when the undistortion map must be rebuilt on the next load. */
    bool _mapDirty = true;

    /** @brief Result of the last load. */
    std::unique_ptr<ImageLoadResult> _imageData;
    /** @brief Watcher of the background load task. */
    QFutureWatcher<std::unique_ptr<ImageLoadResult>> _watcher;
};
