#pragma once

#include <QImage>
#include <QImageIOHandler>

class QtAliceVisionImageIOHandler : public QImageIOHandler
{
  public:
    QtAliceVisionImageIOHandler();
    ~QtAliceVisionImageIOHandler();

    bool canRead() const override;
    bool read(QImage* image) override;
    bool write(const QImage& image) override;

    QByteArray name() const;

    static bool canRead(QIODevice* device);

    QVariant option(ImageOption option) const override;
    void setOption(ImageOption option, const QVariant& value) override;
    bool supportsOption(ImageOption option) const override;

    QSize _scaledSize;
};
