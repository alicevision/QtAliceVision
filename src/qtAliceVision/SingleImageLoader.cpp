#include "SingleImageLoader.hpp"

#include <QThreadPool>

#include <stdexcept>
#include <iostream>

#include <boost/filesystem.hpp>

namespace qtAliceVision {
namespace imgserve {

SingleImageLoader::SingleImageLoader(QObject* parent)
  : QObject(parent)
{
    // Initialize internal state
    _loading = false;
}

SingleImageLoader::~SingleImageLoader() {}

ResponseData SingleImageLoader::request(const RequestData& reqData)
{
    // Check if requested image matches currently loaded image
    if (reqData.path == _request.path && reqData.downscale == _request.downscale)
    {
        return _response;
    }

    // If there is not already a worker thread
    // start a new one using the currently requested image
    if (!_loading)
    {
        // Update internal state
        _loading = true;

        // Create new runnable and launch it in worker thread (managed by Qt thread pool)
        auto ioRunnable = new SingleImageLoadingIORunnable(reqData);
        connect(ioRunnable, &SingleImageLoadingIORunnable::done, this, &SingleImageLoader::onSingleImageLoadingDone);
        QThreadPool::globalInstance()->start(ioRunnable);
    }

    // Empty response
    return ResponseData();
}

void SingleImageLoader::onSingleImageLoadingDone(RequestData reqData, ResponseData response)
{
    // Update internal state
    _loading = false;
    _request = reqData;
    _response = response;

    // Notify listeners that an image has been loaded
    Q_EMIT requestHandled();
}

SingleImageLoadingIORunnable::SingleImageLoadingIORunnable(const RequestData& reqData)
  : _reqData(reqData)
{}

SingleImageLoadingIORunnable::~SingleImageLoadingIORunnable() {}

void SingleImageLoadingIORunnable::run()
{
    ResponseData response;

    try
    {
        // Retrieve metadata from disk
        int width, height;
        auto metadata = aliceVision::image::readImageMetadata(_reqData.path, width, height);

        // Store original image dimensions
        response.dim = QSize(width, height);

        // Copy metadata into a QVariantMap
        for (const auto& item : metadata)
        {
            response.metadata[QString::fromStdString(item.name().string())] = QString::fromStdString(item.get_string());
        }

        // Load image
        response.img = std::make_shared<aliceVision::image::Image<aliceVision::image::RGBAfColor>>();
        aliceVision::image::readImage(_reqData.path, *(response.img), aliceVision::image::EImageColorSpace::LINEAR);

        // Apply downscale
        if (_reqData.downscale > 1)
        {
            aliceVision::imageAlgo::resizeImage(_reqData.downscale, *(response.img));
        }
    }
    catch (const std::runtime_error& e)
    {
        // std::runtime_error at this point is a "can't find/open image" error
        if (!boost::filesystem::exists(_reqData.path))  // "can't find image" case
        {
            response.error = MISSING_FILE;
        }
        else  // "can't open image" case
        {
            response.error = ERROR;
        }

        // Log error message
        std::cerr << e.what() << std::endl;
    }

    // Notify listeners that loading is finished
    Q_EMIT done(_reqData, response);
}

}  // namespace imgserve
}  // namespace qtAliceVision

#include "SingleImageLoader.moc"
