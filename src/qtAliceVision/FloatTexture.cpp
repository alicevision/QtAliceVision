#include "FloatTexture.hpp"
#include <aliceVision/image/resampling.hpp>

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <rhi/qrhi.h>

#include <QtDebug>

namespace qtAliceVision {
int FloatTexture::_maxTextureSize = -1;

FloatTexture::FloatTexture() {}

FloatTexture::~FloatTexture()
{
    if (_rhiTexture)
    {
        _rhiTexture->destroy();
    }
}

void FloatTexture::setImage(std::shared_ptr<FloatImage>& image)
{
    _srcImage = image;
    _textureSize = {_srcImage->width(), _srcImage->height()};
    _dirty = true;
    _mipmapsGenerated = false;
}

bool FloatTexture::isValid() const { return _srcImage->width() != 0 && _srcImage->height() != 0; }

qint64 FloatTexture::comparisonKey() const { return _rhiTexture ? _rhiTexture->nativeTexture().object : 0; }

QRhiTexture* FloatTexture::rhiTexture() const { return _rhiTexture; }

void FloatTexture::commitTextureOperations(QRhi* rhi, QRhiResourceUpdateBatch* resourceUpdates)
{
    if (!_dirty)
    {
        return;
    }

    if (!isValid())
    {
        if (_rhiTexture)
        {
            _rhiTexture->destroy();
        }
        _rhiTexture = nullptr;
        return;
    }

    QRhiTexture::Format texFormat = QRhiTexture::RGBA32F;
    if (!rhi->isTextureFormatSupported(texFormat))
    {
        qWarning() << "[QtAliceVision] Unsupported float images.";
        return;
    }

    // Init max texture size
    if (_maxTextureSize == -1)
    {
        _maxTextureSize = rhi->resourceLimit(QRhi::TextureSizeMax);
    }

    // Downscale the texture to fit inside the max texture limit if it is too big.
    // Downscale into a copy owned by this texture: the source image is shared with the viewer and the image cache.
    while (_maxTextureSize != -1 && (_srcImage->width() > _maxTextureSize || _srcImage->height() > _maxTextureSize))
    {
        auto downscaled = std::make_shared<FloatImage>();
        aliceVision::image::imageHalfSample(*_srcImage, *downscaled);
        _srcImage = std::move(downscaled);
    }
    _textureSize = {_srcImage->width(), _srcImage->height()};

    const QRhiTexture::Flags texFlags(hasMipmaps() ? QRhiTexture::MipMapped : 0);
    _rhiTexture = rhi->newTexture(texFormat, _textureSize, 1, texFlags);
    if (!_rhiTexture || !_rhiTexture->create())
    {
        qWarning() << "[QtAliceVision] Unable to create float texture.";
        return;
    }

    // Reference the pixels instead of copying them: the image is kept alive by this texture,
    // which the scene graph does not release before the upload batch is submitted
    const QByteArray textureData =
      QByteArray::fromRawData(reinterpret_cast<const char*>(_srcImage->data()), _srcImage->size() * sizeof(*_srcImage->data()));
    resourceUpdates->uploadTexture(_rhiTexture, QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(textureData)));

    if (hasMipmaps())
    {
        resourceUpdates->generateMips(_rhiTexture);
        _mipmapsGenerated = true;
    }
    _dirty = false;
}

}  // namespace qtAliceVision
