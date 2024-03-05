#include "FloatTexture.hpp"
#include <aliceVision/image/resampling.hpp>

#include <QOpenGLContext>
#include <QOpenGLFunctions>

//#include <private/qrhi_p.h>
//#include <QRhiResource>
//#include <QRhiTexture>
#include <rhi/qrhi.h>



#include <QtDebug>

namespace qtAliceVision {
int FloatTexture::_maxTextureSize = -1;

FloatTexture::FloatTexture() {}

FloatTexture::~FloatTexture()
{
    /*if (_textureId && QOpenGLContext::currentContext())
    {
        QOpenGLContext::currentContext()->functions()->glDeleteTextures(1, &_textureId);
    }*/
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
    //_dirtyBindOptions = true;
    _mipmapsGenerated = false;
}

bool FloatTexture::isValid() const { return _srcImage->width() != 0 && _srcImage->height() != 0; }

//int FloatTexture::textureId() const
//{
//    if (_dirty)
//    {
//        if (!isValid())
//        {
//            return 0;
//        }
//        else if (_textureId == 0)
//        {
//            QOpenGLContext::currentContext()->functions()->glGenTextures(1, &const_cast<FloatTexture*>(this)->_textureId);
//            return static_cast<int>(_textureId);
//        }
//    }
//    return static_cast<int>(_textureId);
//}
qint64 FloatTexture::comparisonKey() const { return _rhiTexture ? _rhiTexture->nativeTexture().object : 0; }

QRhiTexture* FloatTexture::rhiTexture() const { return _rhiTexture; }

//void FloatTexture::bind()
//{
//    QOpenGLContext* context = QOpenGLContext::currentContext();
//    QOpenGLFunctions* funcs = context->functions();
//    if (!_dirty)
//    {
//        funcs->glBindTexture(GL_TEXTURE_2D, _textureId);
//        if (mipmapFiltering() != QSGTexture::None && !_mipmapsGenerated)
//        {
//            funcs->glGenerateMipmap(GL_TEXTURE_2D);
//            _mipmapsGenerated = true;
//        }
//        updateBindOptions(_dirtyBindOptions);
//        _dirtyBindOptions = false;
//        return;
//    }
//
//    _dirty = false;
//
//    if (!isValid())
//    {
//        if (_textureId)
//        {
//            funcs->glDeleteTextures(1, &_textureId);
//        }
//        _textureId = 0;
//        _textureSize = QSize();
//        return;
//    }
//
//    try
//    {
//        if (_textureId == 0)
//        {
//            funcs->glGenTextures(1, &_textureId);
//        }
//        funcs->glBindTexture(GL_TEXTURE_2D, _textureId);
//
//        // Init max texture size
//        if (_maxTextureSize == -1)
//        {
//            funcs->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_maxTextureSize);
//        }
//
//        // Downscale the texture to fit inside the max texture limit if it is too big.
//        while (_maxTextureSize != -1 && (_srcImage->width() > _maxTextureSize || _srcImage->height() > _maxTextureSize))
//        {
//            FloatImage tmp;
//            aliceVision::image::imageHalfSample(*_srcImage, tmp);
//            *_srcImage = std::move(tmp);
//        }
//        _textureSize = {_srcImage->width(), _srcImage->height()};
//
//        updateBindOptions(_dirtyBindOptions);
//
//        funcs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _textureSize.width(), _textureSize.height(), 0, GL_RGBA, GL_FLOAT, _srcImage->data());
//
//        if (mipmapFiltering() != QSGTexture::None)
//        {
//            funcs->glGenerateMipmap(GL_TEXTURE_2D);
//            _mipmapsGenerated = true;
//        }
//
//        _dirtyBindOptions = false;
//    }
//    catch (std::exception& e)
//    {
//        qInfo() << "[QtAliceVision] Failed to bind image texture: "
//                << "\n"
//                << e.what();
//        _dirty = true;
//    }
//}

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
    while (_maxTextureSize != -1 && (_srcImage->width() > _maxTextureSize || _srcImage->height() > _maxTextureSize))
    {
        FloatImage tmp;
        aliceVision::image::imageHalfSample(*_srcImage, tmp);
        *_srcImage = std::move(tmp);
    }
    _textureSize = {_srcImage->width(), _srcImage->height()};

    const QRhiTexture::Flags texFlags(hasMipmaps() ? QRhiTexture::MipMapped : 0);
    _rhiTexture = rhi->newTexture(texFormat, _textureSize, 1, texFlags);
    if (!_rhiTexture || !_rhiTexture->create())
    {
        qWarning() << "[QtAliceVision] Unable to create float texture.";
        return;
    }

    const QByteArray textureData(reinterpret_cast<const char*>(_srcImage->data()), _srcImage->size() * sizeof(*_srcImage->data()));
    resourceUpdates->uploadTexture(_rhiTexture, QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(textureData)));

    if (hasMipmaps())
    {
        resourceUpdates->generateMips(_rhiTexture);
        _mipmapsGenerated = true;
    }
    _dirty = false;
}

}  // namespace qtAliceVision
