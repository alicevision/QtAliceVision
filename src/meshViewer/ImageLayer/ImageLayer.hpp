#pragma once

#include <Core/LayerItem.hpp>

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/pixelTypes.hpp>

#include <QFutureWatcher>
#include <QString>
#include <memory>

struct ImageLoadResult
{
    aliceVision::image::Image<aliceVision::image::RGBAfColor> image;
    QString errorString;
    bool valid = false;
};

/** @brief Layer that asynchronously loads an EXR/image file for full-screen overlay rendering. */
class ImageLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

  public:
    explicit ImageLayer(QObject* parent = nullptr);
    ~ImageLayer() override;

    QString source() const
    {
        return _source;
    }
    void setSource(const QString& path);

    bool loading() const
    {
        return _loading;
    }
    QString errorString() const
    {
        return _errorString;
    }

    bool imageDirty() const
    {
        return _imageDirty;
    }
    aliceVision::image::Image<aliceVision::image::RGBAfColor> takeImage();

  signals:
    void sourceChanged();
    void loadingChanged();
    void errorStringChanged();

  public:
    std::unique_ptr<IRenderable> createRenderable() const override;

  private slots:
    void onLoadFinished();

  private:
    QString _source;
    bool _loading = false;
    QString _errorString;
    bool _imageDirty = false;
    std::unique_ptr<ImageLoadResult> _imageData;
    QFutureWatcher<std::unique_ptr<ImageLoadResult>> _watcher;
};
