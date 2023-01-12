#include "QtAliceVisionImageIOHandler.hpp"

#include <QImage>
#include <QIODevice>
#include <QFileDevice>
#include <QVariant>
#include <QDataStream>
#include <QDebug>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include <aliceVision/image/io.hpp>
#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/pixelTypes.hpp>

#include <iostream>
#include <memory>


inline const float& clamp(const float& v, const float& lo, const float& hi)
{
    assert(!(hi < lo));
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

inline unsigned short floatToUShort(float v)
{
    return static_cast<unsigned short>(clamp(v, 0.0f, 1.0f) * 65535);
}

QtAliceVisionImageIOHandler::QtAliceVisionImageIOHandler()
{
    qDebug() << "[QtAliceVisionImageIO] QtAliceVisionImageIOHandler";
}

QtAliceVisionImageIOHandler::~QtAliceVisionImageIOHandler()
{
}

bool QtAliceVisionImageIOHandler::canRead() const
{
    if(canRead(device()))
    {
        setFormat(name());
        return true;
    }
    return false;
}

bool QtAliceVisionImageIOHandler::canRead(QIODevice *device)
{
    QFileDevice* d = dynamic_cast<QFileDevice*>(device);
    if(!d)
    {
        qDebug() << "[QtAliceVisionImageIO] Cannot read: invalid device";
        return false;
    }

    const std::string path = d->fileName().toStdString();
    auto input = oiio::ImageInput::create(path);
    if (!input)
    {
        qDebug() << "[QtAliceVisionImageIO] Cannot read: failed to create image input";
        return false;
    }

    if (!(input->valid_file(path)))
    {
        qDebug() << "[QtAliceVisionImageIO] Cannot read: invalid file";
        return false;
    }

    qDebug() << "[QtAliceVisionImageIO] Can read file: " << d->fileName();
    return true;
}

bool QtAliceVisionImageIOHandler::read(QImage *image)
{
    QFileDevice* d = dynamic_cast<QFileDevice*>(device());
    if(!d)
    {
        qWarning() << "[QtAliceVisionImageIO] Read image failed (not a FileDevice).";
        return false;
    }
    const std::string path = d->fileName().toStdString();

    qDebug() << "[QtAliceVisionImageIO] Read image: " << path.c_str();
    aliceVision::image::Image<aliceVision::image::RGBColor> img;
    aliceVision::image::readImage(path, img, aliceVision::image::EImageColorSpace::SRGB);

    oiio::ImageBuf inBuf;
    aliceVision::image::getBufferFromImage(img, inBuf);

    oiio::ImageSpec inSpec = aliceVision::image::readImageSpec(path);
    float pixelAspectRatio = inSpec.get_float_attribute("PixelAspectRatio", 1.0f);

    qDebug() << "[QtAliceVisionImageIO] width:" << inSpec.width
            << ", height:" << inSpec.height
            << ", nchannels:" << inSpec.nchannels
            << ", pixelAspectRatio:" << pixelAspectRatio;

    qDebug() << "[QtAliceVisionImageIO] create output QImage";
    QImage result(inSpec.width, inSpec.height, QImage::Format_RGB32);

    qDebug() << "[QtAliceVisionImageIO] shuffle channels";
    const int nchannels = 4;
    const oiio::TypeDesc typeDesc = oiio::TypeDesc::UINT8;
    oiio::ImageSpec requestedSpec(inSpec.width, inSpec.height, nchannels, typeDesc);
    oiio::ImageBuf tmpBuf(requestedSpec);
    int channelOrder[] = {2, 1, 0, -1};
    float channelValues[] = {1.f, 1.f, 1.f, 1.f};
    oiio::ImageBufAlgo::channels(tmpBuf, inBuf, 4, channelOrder, channelValues, {}, false);
    inBuf.swap(tmpBuf);

    qDebug() << "[QtAliceVisionImageIO] fill output QImage";
    oiio::ROI exportROI = inBuf.roi();
    exportROI.chbegin = 0;
    exportROI.chend = nchannels;
    inBuf.get_pixels(exportROI, typeDesc, result.bits());

    if (pixelAspectRatio != 1.0f)
    {
        QSize newSize(static_cast<int>(static_cast<float>(inSpec.width) * pixelAspectRatio), inSpec.height);
        result = result.scaled(newSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    if (_scaledSize.isValid())
    {
        qDebug() << "[QtAliceVisionImageIO] _scaledSize: " << _scaledSize.width() << "x" << _scaledSize.height();
        *image = result.scaled(_scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    else
    {
        *image = result;
    }
    return true;
}

bool QtAliceVisionImageIOHandler::write(const QImage&)
{
    // TODO
    return false;
}

bool QtAliceVisionImageIOHandler::supportsOption(ImageOption option) const
{
    if(option == Size)
        return true;
    if(option == ImageTransformation)
        return true;
    if(option == ScaledSize)
        return true;

    return false;
}

QVariant QtAliceVisionImageIOHandler::option(ImageOption option) const
{
    QFileDevice* d = dynamic_cast<QFileDevice*>(device());
    if(!d)
    {
        qDebug("[QtAliceVisionImageIO] Read image failed (not a FileDevice).");
        return QImageIOHandler::option(option);
    }
    std::string path = d->fileName().toStdString();
    oiio::ImageSpec spec = aliceVision::image::readImageSpec(path);

    if (option == Size)
    {
        return QSize(spec.width, spec.height);
    }
    else if(option == ImageTransformation)
    {
        int orientation = 0;
        spec.getattribute("orientation", oiio::TypeInt, &orientation);
        switch(orientation)
        {
        case 1: return QImageIOHandler::TransformationNone; break;
        case 2: return QImageIOHandler::TransformationMirror; break;
        case 3: return QImageIOHandler::TransformationRotate180; break;
        case 4: return QImageIOHandler::TransformationFlip; break;
        case 5: return QImageIOHandler::TransformationFlipAndRotate90; break;
        case 6: return QImageIOHandler::TransformationRotate90; break;
        case 7: return QImageIOHandler::TransformationMirrorAndRotate90; break;
        case 8: return QImageIOHandler::TransformationRotate270; break;
        default: break;
        }
    }
    return QImageIOHandler::option(option);
}

void QtAliceVisionImageIOHandler::setOption(ImageOption option, const QVariant &value)
{
    Q_UNUSED(option);
    Q_UNUSED(value);
    if (option == ScaledSize && value.isValid())
    {
        _scaledSize = value.value<QSize>();
        qDebug() << "[QtAliceVisionImageIO] setOption scaledSize: " << _scaledSize.width() << "x" << _scaledSize.height();
    }
}

QByteArray QtAliceVisionImageIOHandler::name() const
{
    return "AliceVisionImageIO";
}
