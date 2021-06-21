#include "plugin.hpp"
#include "QtAliceVisionImageIOHandler.hpp"

#include <QImageIOHandler>
#include <QFileDevice>
#include <QDebug>

#include <OpenImageIO/imageio.h>

#include <iostream>

namespace oiio = OIIO;

namespace qtAliceVisionImageIOHandler
{

QtAliceVisionImageIOHandlerPlugin::QtAliceVisionImageIOHandlerPlugin()
{
    qDebug() << "[QtAliceVisionImageIOHandler] initialize supported extensions.";

    std::string extensionsListStr;
    oiio::getattribute("extension_list", extensionsListStr);

    QString extensionsListQStr(QString::fromStdString(extensionsListStr));

    QStringList formats = extensionsListQStr.split(';');
    for(auto& format: formats)
    {
        qDebug() << "[QtAliceVisionImageIOHandler] format: " << format << ".";
        QStringList keyValues = format.split(":");
        if(keyValues.size() != 2)
        {
            qDebug() << "[QtAliceVisionImageIOHandler] warning: split OIIO keys: " << keyValues.size() << " for " << format << ".";
        }
        else
        {
            _supportedExtensions += keyValues[1].split(",");
        }
    }
    qDebug() << "[QtAliceVisionImageIOHandler] supported extensions: " << _supportedExtensions.join(", ");
    qInfo() << "[QtAliceVisionImageIOHandler] Plugin Initialized";
}

QtAliceVisionImageIOHandlerPlugin::~QtAliceVisionImageIOHandlerPlugin()
{
}

QImageIOPlugin::Capabilities QtAliceVisionImageIOHandlerPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    QFileDevice* d = dynamic_cast<QFileDevice*>(device);
    if(!d)
        return QImageIOPlugin::Capabilities();

    const std::string path = d->fileName().toStdString();
    if(path.empty() || path[0] == ':')
        return QImageIOPlugin::Capabilities();

#ifdef QTALICEVISION_IMAGEIO_HANDLER_USE_FORMATS_BLACKLIST 
    // For performance sake, let Qt handle natively some formats.
    static const QStringList blacklist{"jpeg", "jpg", "png", "ico"};
    if(blacklist.contains(format, Qt::CaseSensitivity::CaseInsensitive))
    {
        return QImageIOPlugin::Capabilities();
    }
#endif
    if (_supportedExtensions.contains(format, Qt::CaseSensitivity::CaseInsensitive))
    {
        qDebug() << "[QtAliceVisionImageIOHandler] Capabilities: extension \"" << QString(format) << "\" supported.";
//        oiio::ImageOutput *out = oiio::ImageOutput::create(path); // TODO: when writting will be implemented
        Capabilities capabilities(CanRead);
//        if(out)
//            capabilities = Capabilities(CanRead|CanWrite);
//        oiio::ImageOutput::destroy(out);
        return capabilities;
    }
    qDebug() << "[QtAliceVisionImageIOHandler] Capabilities: extension \"" << QString(format) << "\" not supported";
    return QImageIOPlugin::Capabilities();
}

QImageIOHandler *QtAliceVisionImageIOHandlerPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QtAliceVisionImageIOHandler* handler = new QtAliceVisionImageIOHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}

} // namespace qtAliceVisionImageIOHandler