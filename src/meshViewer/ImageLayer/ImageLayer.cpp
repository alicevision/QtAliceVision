#include <ImageLayer/ImageLayer.hpp>
#include <ImageLayer/ImageRenderable.hpp>

#include <aliceVision/image/io.hpp>

#include <QtConcurrent/QtConcurrent>
#include <QUrl>

namespace {

std::unique_ptr<ImageLoadResult> loadImageFile(const QString& path)
{
    auto result = std::make_unique<ImageLoadResult>();
    try
    {
        aliceVision::image::ImageReadOptions opts;
        opts.workingColorSpace = aliceVision::image::EImageColorSpace::NO_CONVERSION;
        aliceVision::image::readImage(path.toStdString(), result->image, opts);
        result->valid = true;
    }
    catch (const std::exception& e)
    {
        result->errorString = QString::fromStdString(e.what());
    }
    return result;
}

}  // namespace

ImageLayer::ImageLayer(QObject* parent)
  : LayerItem(parent)
{
    connect(&_watcher, &QFutureWatcher<std::unique_ptr<ImageLoadResult>>::finished, this, &ImageLayer::onLoadFinished);
}

ImageLayer::~ImageLayer()
{
    if (_watcher.isRunning())
    {
        _watcher.waitForFinished();
    }
}

void ImageLayer::setSource(const QString& path)
{
    if (_source == path)
    {
        return;
    }

    _source = path;
    emit sourceChanged();

    if (path.isEmpty())
    {
        return;
    }

    QString filePath = path;
    if (filePath.startsWith("file://"))
    {
        filePath = QUrl(filePath).toLocalFile();
    }

    _loading = true;
    emit loadingChanged();

    _watcher.setFuture(QtConcurrent::run([filePath]() { return loadImageFile(filePath); }));
}

void ImageLayer::onLoadFinished()
{
    _imageData = _watcher.future().takeResult();
    _loading = false;

    if (!_imageData->valid)
    {
        _errorString = _imageData->errorString;
    }
    else
    {
        _errorString.clear();
        _imageDirty = true;
        emit dataReady();
    }

    emit loadingChanged();
    emit errorStringChanged();
}

aliceVision::image::Image<aliceVision::image::RGBAfColor> ImageLayer::takeImage()
{
    _imageDirty = false;
    return std::move(_imageData->image);
}

std::unique_ptr<IRenderable> ImageLayer::createRenderable() const
{
    return std::make_unique<ImageRenderable>();
}
