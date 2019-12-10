#pragma once

#include <aliceVision/types.hpp>
#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/pixelTypes.hpp>

#include <QSGTexture>


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

    void setImage(const FloatImage &image);
    const FloatImage &image() { return _srcImage; }

    void bind() override;

private:
    bool isValid() const;

private:
    FloatImage _srcImage;

    uint _textureId = 0;
    QSize _textureSize;

    bool _dirty = false;
    bool _dirtyBindOptions = false;
    bool _mipmapsGenerated = false;
};

}  // ns qtAliceVision
