#include "SingleImageLoader.hpp"

#include <QThreadPool>

#include <stdexcept>
#include <iostream>


namespace qtAliceVision {
namespace imgserve {

SingleImageLoader::SingleImageLoader(QObject* parent) : 
	QObject(parent)
{
	// Initialize internal state
	_loading = false;
}

SingleImageLoader::~SingleImageLoader()
{}

Response SingleImageLoader::request(const std::string& path)
{
	// Check if requested image matches currently loaded image
	if (path == _path)
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
		auto ioRunnable = new SingleImageLoadingIORunnable(path);
        connect(ioRunnable, &SingleImageLoadingIORunnable::done, this, &SingleImageLoader::onSingleImageLoadingDone);
        QThreadPool::globalInstance()->start(ioRunnable);
	}

	// Empty response
	return Response();
}

void SingleImageLoader::onSingleImageLoadingDone(QString path, Response response)
{
	// Update internal state
	_loading = false;
	_path = path.toStdString();
	_response = response;

	// Notify listeners that an image has been loaded
	Q_EMIT requestHandled();
}

SingleImageLoadingIORunnable::SingleImageLoadingIORunnable(const std::string& path) : 
	_path(path)
{}

SingleImageLoadingIORunnable::~SingleImageLoadingIORunnable()
{}

void SingleImageLoadingIORunnable::run()
{
	Response response;

    try
    {
        // Retrieve metadata from disk
        int width, height;
        auto metadata = aliceVision::image::readImageMetadata(_path, width, height);

        // Store original image dimensions
        response.dim = QSize(width, height);

        // Copy metadata into a QVariantMap
        for (const auto& item : metadata)
        {
            response.metadata[QString::fromStdString(item.name().string())] = QString::fromStdString(item.get_string());
        }

        // Load image
        response.img = std::make_shared<aliceVision::image::Image<aliceVision::image::RGBAfColor>>();
        aliceVision::image::readImage(_path, *(response.img), aliceVision::image::EImageColorSpace::LINEAR);
    }
    catch (const std::runtime_error& e)
    {
        // Log error message
        std::cerr << e.what() << std::endl;
    }

    // Notify listeners that loading is finished
    Q_EMIT done(QString::fromStdString(_path), response);
}

} // namespace imgserve
} // namespace qtAliceVision

#include "SingleImageLoader.moc"
