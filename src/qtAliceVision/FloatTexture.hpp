#pragma once

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/pixelTypes.hpp>
#include <aliceVision/types.hpp>

#include <QSGTexture>
#include <memory>

namespace qtAliceVision {

using FloatImage = aliceVision::image::Image<aliceVision::image::RGBAfColor>;

/**
 * @brief A custom QSGTexture to display AliceVision images in QML
 */
class FloatTexture : public QSGTexture
{
  public:
    FloatTexture();
    ~FloatTexture() override;

    virtual qint64 comparisonKey() const override;
    virtual QRhiTexture* rhiTexture() const override;
    virtual void commitTextureOperations(QRhi* rhi, QRhiResourceUpdateBatch* resourceUpdates) override;

    QSize textureSize() const override { return _textureSize; }

    bool hasAlphaChannel() const override { return true; }

    bool hasMipmaps() const override { return mipmapFiltering() != QSGTexture::None; }

    void setImage(std::shared_ptr<FloatImage>& image);
    const FloatImage& image() { return *_srcImage; }

    /**
     * @brief Get the maximum dimension of a texture.
     *
     * If the provided image is too large, it will be downscaled to fit the max dimension.
     *
     * @return -1 if unknown else the max size of a texture
     */
    static int maxTextureSize() { return _maxTextureSize; }

  private:
    bool isValid() const;

  private:
    std::shared_ptr<FloatImage> _srcImage;

    QRhiTexture* _rhiTexture = nullptr;
    QSize _textureSize;

    bool _dirty = false;
    bool _mipmapsGenerated = false;

    static int _maxTextureSize;
};

}  // namespace qtAliceVision
