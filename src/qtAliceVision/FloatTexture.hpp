#pragma once

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/pixelTypes.hpp>
#include <aliceVision/types.hpp>

#include <QSGTexture>
#include <functional>
#include <memory>

namespace qtAliceVision {

/** @brief 32-bit float RGBA image type used throughout the viewer pipeline. */
using FloatImage = aliceVision::image::Image<aliceVision::image::RGBAfColor>;

/**
 * @brief A custom QSGTexture that uploads an AliceVision floating-point image to the GPU for display in QML.
 *
 * The texture is marked dirty whenever a new image is set and uploads its data
 * to the RHI backend during the next commitTextureOperations() call.
 */
class FloatTexture : public QSGTexture
{
  public:
    FloatTexture();
    ~FloatTexture() override;

    /** @brief Returns an integer key used by the scene graph to compare and deduplicate textures. */
    qint64 comparisonKey() const override;

    /** @brief Returns the underlying QRhiTexture, or nullptr if not yet uploaded. */
    QRhiTexture* rhiTexture() const override;

    /**
     * @brief Uploads pending image data to the GPU via the provided RHI resource update batch.
     * @param rhi The RHI instance managing GPU resources.
     * @param resourceUpdates Batch to which upload commands are appended.
     */
    void commitTextureOperations(QRhi* rhi, QRhiResourceUpdateBatch* resourceUpdates) override;

    /**
     * @brief Returns the size of the uploaded texture in pixels.
     * @return Texture dimensions; QSize() if no image has been uploaded yet.
     */
    QSize textureSize() const override { return _textureSize; }

    /** @brief Returns true; this texture always carries an alpha channel. */
    bool hasAlphaChannel() const override { return true; }

    /** @brief Returns true if mipmap filtering is enabled on the texture. */
    bool hasMipmaps() const override { return mipmapFiltering() != QSGTexture::None; }

    /**
     * @brief Sets the source image to be uploaded on the next commitTextureOperations() call.
     * @param image Shared pointer to the floating-point RGBA source image.
     */
    void setImage(const std::shared_ptr<FloatImage>& image);

    /**
     * @brief Registers a callback invoked on the render thread after commitTextureOperations()
     *        finishes, passing the final (possibly downscaled) texture size.
     *
     * Use QMetaObject::invokeMethod with Qt::QueuedConnection inside the callback to
     * safely marshal work back to the GUI thread.
     */
    void setOnCommit(std::function<void(QSize)> callback) { _onCommit = std::move(callback); }

    /**
     * @brief Returns a read-only reference to the currently assigned source image.
     * @return Const reference to the FloatImage held by this texture.
     */
    const FloatImage& image() const { return *_srcImage; }

    /**
     * @brief Returns the maximum supported texture dimension (width or height) in pixels.
     *
     * If the source image exceeds this dimension it will be downscaled before upload.
     *
     * @return Maximum texture size in pixels, or -1 if the limit has not yet been queried from the GPU.
     */
    static int maxTextureSize() { return _maxTextureSize; }

  private:
    /**
     * @brief Returns true if the texture has a valid RHI texture object and a non-empty source image.
     */
    bool isValid() const;

    std::shared_ptr<FloatImage> _srcImage;
    std::function<void(QSize)> _onCommit;

    /**
     * @brief Custom deleter for the QRhiTexture unique_ptr.
     *
     * QRhiTexture follows a two-step teardown: destroy() releases the underlying GPU
     * resource (framebuffer object, Vulkan image, etc.) while the C++ wrapper object
     * itself must be freed separately with delete. Using this deleter as the second
     * template argument of std::unique_ptr ensures both steps always happen together,
     * preventing both GPU resource leaks and C++ heap leaks on every image update and
     * at object destruction.
     */
    struct RhiTextureDeleter
    {
      void operator()(QRhiTexture* t) const;
    };
    std::unique_ptr<QRhiTexture, RhiTextureDeleter> _rhiTexture;
    QSize _textureSize;

    bool _dirty = false;
    bool _mipmapsGenerated = false;

    /** @brief GPU-queried maximum texture dimension. -1 until first queried. */
    static int _maxTextureSize;
};

}  // namespace qtAliceVision
