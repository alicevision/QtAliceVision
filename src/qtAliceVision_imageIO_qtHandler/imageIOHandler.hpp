#pragma once

#include <QImageIOHandler>
#include <QImage>

class QtAliceVisionImageIOHandler : public QImageIOHandler
{
public:
    QtAliceVisionImageIOHandler();
    ~QtAliceVisionImageIOHandler();

    bool canRead() const;
    bool read(QImage *image);
    bool write(const QImage &image);

    QByteArray name() const;

    static bool canRead(QIODevice *device);

    QVariant option(ImageOption option) const;
    void setOption(ImageOption option, const QVariant &value);
    bool supportsOption(ImageOption option) const;

    QSize _scaledSize;
};
