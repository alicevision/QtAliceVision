#include "plugin.hpp"
#include "QtAliceVisionImageIOHandler.hpp"

#include <QFileDevice>
#include <QDebug>

#include <aliceVision/image/io.hpp>

#include <iostream>

using namespace aliceVision;

QtAliceVisionImageIOPlugin::QtAliceVisionImageIOPlugin()
{
    qDebug("[QtAliceVisionImageIO] init supported extensions.");

    std::vector<std::string> extensions = image::getSupportedExtensions();
    for(auto& ext: extensions)
    {
        QString format(ext.c_str());
        format.remove('.');
        qDebug() << "[QtAliceVisionImageIO] supported format: " << format;
        _supportedExtensions.append(format);
    }
    qInfo() << "[QtAliceVisionImageIO] Plugin Initialized";
}

QtAliceVisionImageIOPlugin::~QtAliceVisionImageIOPlugin()
{
}

QImageIOPlugin::Capabilities QtAliceVisionImageIOPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    QFileDevice* d = dynamic_cast<QFileDevice*>(device);
    if(!d)
        return QImageIOPlugin::Capabilities();

    const std::string path = d->fileName().toStdString();
    if(path.empty() || path[0] == ':')
        return QImageIOPlugin::Capabilities();

#ifdef QT_ALICEVISIONIMAGEIO_USE_FORMATS_BLACKLIST
    // For performance sake, let Qt handle natively some formats.
    static const QStringList blacklist{"jpeg", "jpg", "png", "ico"};
    if(blacklist.contains(format, Qt::CaseSensitivity::CaseInsensitive))
    {
        return QImageIOPlugin::Capabilities();
    }
#endif
    if (_supportedExtensions.contains(format, Qt::CaseSensitivity::CaseInsensitive))
    {
        qDebug() << "[QtAliceVisionImageIO] Capabilities: extension \"" << QString(format) << "\" supported.";
        Capabilities capabilities(CanRead);
        return capabilities;
    }
    qDebug() << "[QtAliceVisionImageIO] Capabilities: extension \"" << QString(format) << "\" not supported";
    return QImageIOPlugin::Capabilities();
}

QImageIOHandler *QtAliceVisionImageIOPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QtAliceVisionImageIOHandler *handler = new QtAliceVisionImageIOHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}
