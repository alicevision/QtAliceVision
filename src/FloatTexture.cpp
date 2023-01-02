#include "FloatTexture.hpp"
#include <aliceVision/image/resampling.hpp>

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <private/qrhi_p.h>

#include <QtDebug>


namespace qtAliceVision
{
int FloatTexture::_maxTextureSize = -1;

FloatTexture::FloatTexture()
{
}

FloatTexture::~FloatTexture()
{
    if (_rhiTexture)
    {
        _rhiTexture->destroy();
    }
}

void FloatTexture::setImage(QSharedPointer<FloatImage>& image)
{
    _srcImage = image;
    _textureSize = { _srcImage->Width(), _srcImage->Height() };
    _dirty = true;
    _mipmapsGenerated = false;
 }

bool FloatTexture::isValid() const
{
    return _srcImage->Width() != 0 && _srcImage->Height() != 0;
}

qint64 FloatTexture::comparisonKey() const
{
    return _rhiTexture ? _rhiTexture->nativeTexture().object : 0;
}

QRhiTexture* FloatTexture::rhiTexture() const
{
    return _rhiTexture;
}

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

    // TODO: support 16bits? QRhiTexture::RGBA16F
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
    while (_maxTextureSize != -1 &&
        (_srcImage->Width() > _maxTextureSize || _srcImage->Height() > _maxTextureSize))
    {
        FloatImage tmp;
        aliceVision::image::ImageHalfSample(*_srcImage, tmp);
        *_srcImage = std::move(tmp);
    }
    _textureSize = { _srcImage->Width(), _srcImage->Height() };

    const QRhiTexture::Flags texFlags(hasMipmaps() ? QRhiTexture::MipMapped : 0);
    _rhiTexture = rhi->newTexture(texFormat, _textureSize, 1, texFlags);
    if (!_rhiTexture || !_rhiTexture->create())
    {
        qWarning() << "[QtAliceVision] Unable to create float texture.";
        return;
    }

    const QByteArray textureData(reinterpret_cast<const char*>(_srcImage->data()), _srcImage->size() * sizeof(*_srcImage->data()));
    resourceUpdates->uploadTexture(
        _rhiTexture,
        QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(textureData)));

    if (hasMipmaps())
    {
        resourceUpdates->generateMips(_rhiTexture);
        _mipmapsGenerated = true;
    }
    _dirty = false;
}

}  // ns qtAliceVision
