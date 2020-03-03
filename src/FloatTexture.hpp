#pragma once

#include <aliceVision/types.hpp>
#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/pixelTypes.hpp>

#include <QSGTexture>

#include <QSharedPointer>


namespace qtAliceVision
{

using FloatImage = aliceVision::image::Image<aliceVision::image::RGBAfColor>;

/**
 * @brief A custom QSGTexture to display AliceVision images in QML
 */
class FloatTexture : public QSGTexture
{
public:
    FloatTexture();
    ~FloatTexture() override;

    int textureId() const override;

    QSize textureSize() const override
    {
        return _textureSize;
    }

    bool hasAlphaChannel() const override { return true; }

    bool hasMipmaps() const override { return mipmapFiltering() != QSGTexture::None; }

    void setImage(QSharedPointer<FloatImage>& image);
    const FloatImage &image() { return *_srcImage; }

    void bind() override;

    /**
     * @brief Get the maximum dimension of a texture.
     *
     * If the provided image is too large, it will be downscaled to fit the max dimension.
     *
     * @return -1 if unknown else the max size of a texture
     */
    static int maxTextureSize()
    {
        return _maxTextureSize;
    }

private:
    bool isValid() const;

private:
    QSharedPointer<FloatImage> _srcImage;

    uint _textureId = 0;
    QSize _textureSize;

    bool _dirty = false;
    bool _dirtyBindOptions = false;
    bool _mipmapsGenerated = false;

    static int _maxTextureSize;
};

}  // ns qtAliceVision
