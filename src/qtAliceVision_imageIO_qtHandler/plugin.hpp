#pragma once

#include <qimageiohandler.h>
#include <QtCore/QtGlobal>

#include <iostream>

// QImageIOHandlerFactoryInterface_iid
// QT_DECL_EXPORT

namespace qtAliceVisionImageIOHandler
{

class QtAliceVisionImageIOHandlerPlugin : public QImageIOPlugin
{
public:
  Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "pluginMetadata.json")

public:
  QStringList _supportedExtensions;

public:
  QtAliceVisionImageIOHandlerPlugin();
  ~QtAliceVisionImageIOHandlerPlugin();

  Capabilities capabilities(QIODevice* device, const QByteArray& format) const;
  QImageIOHandler* create(QIODevice* device, const QByteArray& format = QByteArray()) const;
};

} // namespace qtAliceVisionImageIOHandler