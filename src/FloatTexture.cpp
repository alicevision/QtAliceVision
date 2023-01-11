#include "FloatTexture.hpp"
#include <aliceVision/image/resampling.hpp>

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <QtDebug>


namespace qtAliceVision
{
int FloatTexture::_maxTextureSize = -1;

FloatTexture::FloatTexture()
{
}

FloatTexture::~FloatTexture()
{
    if (_textureId && QOpenGLContext::currentContext())
    {
        QOpenGLContext::currentContext()->functions()->glDeleteTextures(1, &_textureId);
    }
}

void FloatTexture::setImage(QSharedPointer<FloatImage>& image)
{
    _srcImage = image;
    _textureSize = { _srcImage->Width(), _srcImage->Height() };
    _dirty = true;
    _dirtyBindOptions = true;
    _mipmapsGenerated = false;
 }

bool FloatTexture::isValid() const
{
    return _srcImage->Width() != 0 && _srcImage->Height() != 0;
}

int FloatTexture::textureId() const
{
    if(_dirty)
    {
        if(!isValid())
        {
            return 0;
        }
        else if(_textureId == 0)
        {
            QOpenGLContext::currentContext()->functions()->glGenTextures(1, &const_cast<FloatTexture *>(this)->_textureId);
            return (int)_textureId;
        }
    }
    return (int)_textureId;
}

void FloatTexture::bind()
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    QOpenGLFunctions *funcs = context->functions();
    if(!_dirty)
    {
        funcs->glBindTexture(GL_TEXTURE_2D, _textureId);
        if (mipmapFiltering() != QSGTexture::None && !_mipmapsGenerated)
        {
            funcs->glGenerateMipmap(GL_TEXTURE_2D);
            _mipmapsGenerated = true;
        }
        updateBindOptions(_dirtyBindOptions);
        _dirtyBindOptions = false;
        return;
    }

    _dirty = false;

    if(!isValid())
    {
        if(_textureId)
        {
            funcs->glDeleteTextures(1, &_textureId);
        }
        _textureId = 0;
        _textureSize = QSize();
        return;
    }

    try 
    {
        if(_textureId == 0)
        {
            funcs->glGenTextures(1, &_textureId);
        }
        funcs->glBindTexture(GL_TEXTURE_2D, _textureId);

        // Init max texture size
        if(_maxTextureSize == -1)
        {
            funcs->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_maxTextureSize);
        }

        // Downscale the texture to fit inside the max texture limit if it is too big.
        while(_maxTextureSize != -1 &&
            (_srcImage->Width() > _maxTextureSize || _srcImage->Height() > _maxTextureSize))
        {
            FloatImage tmp;
            aliceVision::image::ImageHalfSample(*_srcImage, tmp);
            *_srcImage = std::move(tmp);
        }
        _textureSize = { _srcImage->Width(), _srcImage->Height() };

        updateBindOptions(_dirtyBindOptions);

        funcs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _textureSize.width(), _textureSize.height(), 0, GL_RGBA, GL_FLOAT, _srcImage->data());

        if(mipmapFiltering() != QSGTexture::None)
        {
            funcs->glGenerateMipmap(GL_TEXTURE_2D);
            _mipmapsGenerated = true;
        }

        _dirtyBindOptions = false;
    }
    catch(std::exception& e)
    {
        qInfo() << "[QtAliceVision] Failed to bind image texture: "
                 << "\n" << e.what();
        _dirty = true;
    }
}

}  // ns qtAliceVision
