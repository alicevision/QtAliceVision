#pragma once

#include <QImageIOHandler>
#include <QtCore/QtGlobal>

class QtAliceVisionImageIOPlugin : public QImageIOPlugin
{
  public:
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "QtAliceVisionImageIOPlugin.json")

  public:
    QStringList _supportedExtensions;

  public:
    QtAliceVisionImageIOPlugin();
    ~QtAliceVisionImageIOPlugin();

    Capabilities capabilities(QIODevice* device, const QByteArray& format) const;
    QImageIOHandler* create(QIODevice* device, const QByteArray& format = QByteArray()) const;
};
