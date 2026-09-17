#include <ImageLayer/ImageLayer.hpp>
#include <ImageLayer/ImageRenderable.hpp>

#include <aliceVision/image/io.hpp>
#include <aliceVision/image/Image.hpp>

#include <QtConcurrent/QtConcurrent>
#include <QUrl>

namespace {

std::unique_ptr<ImageLoadResult> loadImageFile(const QString& path, aliceVision::camera::IntrinsicBase* intrinsics, bool buildMap)
{
    auto result = std::make_unique<ImageLoadResult>();
    try
    {
        aliceVision::image::ImageReadOptions opts;
        opts.workingColorSpace = aliceVision::image::EImageColorSpace::SRGB;

        aliceVision::image::readImage(path.toStdString(), result->content.image, opts);
        result->valid = true;
    }
    catch (const std::exception& e)
    {
        result->errorString = QString::fromStdString(e.what());
    }

    if (intrinsics && buildMap)
    {
        if ((intrinsics->w() != result->content.image.width()) || (intrinsics->h() != result->content.image.height()))
        {
            result->errorString = QString::fromStdString("Invalid image size with respect to intrinsics");
            result->valid = false;
        }
    }

    if (!result->valid)
    {
        return result;
    }

    if (!buildMap)
    {
        return result;
    }

    int w = result->content.image.width();
    int h = result->content.image.height();

    result->content.map = aliceVision::image::Image<aliceVision::image::RGBAfColor>(w, h);

    double wm = double(w) - 1;
    double hm = double(h) - 1;

    if (intrinsics)
    {
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const aliceVision::Vec2 undisto_pix(double(x) + 0.5, double(y) + 0.5);

                // compute coordinates with distortion
                aliceVision::Vec2 disto_pix = intrinsics->cam2ima(intrinsics->addDistortion(intrinsics->ima2cam(undisto_pix)));

                disto_pix.x() = disto_pix.x() / wm;
                disto_pix.y() = disto_pix.y() / hm;

                result->content.map(y, x) = aliceVision::image::RGBAfColor(disto_pix.x(), disto_pix.y(), 0.0, 0.0);
            }
        }
    }
    else
    {
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                aliceVision::Vec2 disto_pix;
                disto_pix.x() = double(x) / wm;
                disto_pix.y() = double(y) / wm;

                result->content.map(y, x) = aliceVision::image::RGBAfColor(disto_pix.x(), disto_pix.y(), 0.0, 0.0);
            }
        }
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

    bool buildMap = false;
    aliceVision::camera::IntrinsicBase* lintrinsic = nullptr;
    if (_mapDirty)
    {
        buildMap = true;
        lintrinsic = _intrinsics->clone();
    }

    _watcher.setFuture(QtConcurrent::run([filePath, lintrinsic, buildMap]() { return loadImageFile(filePath, lintrinsic, buildMap); }));
}

void ImageLayer::onLoadFinished()
{
    if (_mapDirty)
    {
        _imageData = _watcher.future().takeResult();
    }
    else
    {
        // Keep old map if nothing changed
        auto oldmap = std::move(_imageData->content.map);
        _imageData = _watcher.future().takeResult();
        _imageData->content.map = std::move(oldmap);
    }

    _loading = false;

    if (!_imageData->valid)
    {
        _errorString = _imageData->errorString;
    }
    else
    {
        _errorString.clear();

        _imageDirty = true;
        _mapDirty = false;

        emit dataReady();
    }

    emit loadingChanged();
    emit errorStringChanged();
}

ImageAndMap ImageLayer::takeImage()
{
    return std::move(_imageData->content);
}

std::unique_ptr<IRenderable> ImageLayer::createRenderable() const
{
    return std::make_unique<ImageRenderable>();
}

void ImageLayer::setIntrinsics(SfmDataObject* object, aliceVision::IndexT viewId)
{
    if (!object)
    {
        return;
    }

    if (!object->valid())
    {
        return;
    }

    auto it = object->content().cameraPerViewId.find(viewId);
    if (it == object->content().cameraPerViewId.end())
    {
        return;
    }

    auto intrinsics = it->second.intrinsics;
    if (!intrinsics)
    {
        return;
    }

    if (_intrinsics)
    {
        if ((*intrinsics) == (*_intrinsics))
        {
            return;
        }
    }

    _intrinsics = std::shared_ptr<aliceVision::camera::IntrinsicBase>(intrinsics->clone());
    _mapDirty = true;
}
