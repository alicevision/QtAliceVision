#include "FloatTexture.hpp"
#include <aliceVision/image/resampling.hpp>

#include <rhi/qrhi.h>

#include <QtDebug>

namespace qtAliceVision {

static constexpr const char* kLogPrefix = "[QtAliceVision] ";

int FloatTexture::_maxTextureSize = -1;

void FloatTexture::RhiTextureDeleter::operator()(QRhiTexture* t) const
{
    if (t)
    {
        t->destroy();
        delete t;
    }
}

FloatTexture::FloatTexture() {}

FloatTexture::~FloatTexture() = default;

void FloatTexture::setImage(const std::shared_ptr<FloatImage>& image)
{
    if (!image || image->width() == 0 || image->height() == 0)
    {
        qWarning() << kLogPrefix << "setImage() called with a null or empty image; ignoring.";
        return;
    }
    _srcImage = image;
    _textureSize = {image->width(), image->height()};
    _dirty = true;
    _mipmapsGenerated = false;
}

bool FloatTexture::isValid() const
{
    return _srcImage && _srcImage->width() != 0 && _srcImage->height() != 0;
}

qint64 FloatTexture::comparisonKey() const
{
    return _rhiTexture ? static_cast<qint64>(_rhiTexture->nativeTexture().object) : 0;
}

QRhiTexture* FloatTexture::rhiTexture() const
{
    return _rhiTexture.get();
}

void FloatTexture::commitTextureOperations(QRhi* rhi, QRhiResourceUpdateBatch* resourceUpdates)
{
    if (!_dirty)
    {
        return;
    }


    if (!isValid())
    {
        _rhiTexture.reset();
        _dirty = false;
        return;
    }

    const QRhiTexture::Format texFormat = QRhiTexture::RGBA32F;
    if (!rhi->isTextureFormatSupported(texFormat))
    {
        qWarning() << kLogPrefix << "Unsupported float texture format; cannot upload image.";
        _dirty = false;
        return;
    }

    // Query the GPU's maximum texture dimension on first use.
    if (_maxTextureSize == -1)
    {
        _maxTextureSize = rhi->resourceLimit(QRhi::TextureSizeMax);
    }

    const FloatImage* uploadImage = _srcImage.get();
    FloatImage scaledImage;
    if (_maxTextureSize != -1 && (_srcImage->width() > _maxTextureSize || _srcImage->height() > _maxTextureSize))
    {
        // Only copy/downscale when the source exceeds GPU limits.
        scaledImage = *_srcImage;
        while (scaledImage.width() > _maxTextureSize || scaledImage.height() > _maxTextureSize)
        {
            FloatImage tmp;
            aliceVision::image::imageHalfSample(scaledImage, tmp);
            scaledImage = std::move(tmp);
        }
        uploadImage = &scaledImage;
    }

    const QSize newTextureSize(uploadImage->width(), uploadImage->height());
    const QRhiTexture::Flags texFlags(hasMipmaps() ? QRhiTexture::MipMapped : 0);

    const bool needsReallocation = !_rhiTexture || _rhiTexture->format() != texFormat || _rhiTexture->pixelSize() != newTextureSize || _rhiTexture->flags() != texFlags;
    if (needsReallocation)
    {
        _rhiTexture.reset();
        _rhiTexture.reset(rhi->newTexture(texFormat, newTextureSize, 1, texFlags));
        if (!_rhiTexture || !_rhiTexture->create())
        {
            qWarning() << kLogPrefix << "Unable to create float texture.";
            _rhiTexture.reset();
            _dirty = false;
            return;
        }
    }

    _textureSize = newTextureSize;

    // Declare texture data from image properties
    const QByteArray textureData(
        reinterpret_cast<const char*>(uploadImage->data()),
        static_cast<qsizetype>(uploadImage->size()) * static_cast<qsizetype>(sizeof(*uploadImage->data())));

    // Upload image data to texture
    resourceUpdates->uploadTexture(_rhiTexture.get(), QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(textureData)));

    if (hasMipmaps())
    {
        resourceUpdates->generateMips(_rhiTexture.get());
        _mipmapsGenerated = true;
    }

    _dirty = false;

    // Notify the owner if the committed size differs from what setImage() reported
    // (e.g. because the image was downscaled to fit the GPU texture size limit).
    if (_onCommit)
        _onCommit(_textureSize);
}

}  // namespace qtAliceVision
