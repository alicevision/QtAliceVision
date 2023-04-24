#include "SingleImageLoader.hpp"

#include <QThreadPool>
#include <QString>


namespace qtAliceVision {
namespace imageio {

SingleImageLoader::SingleImageLoader(QObject* parent) : 
	QObject(parent)
{
	_loading = false;
}

SingleImageLoader::~SingleImageLoader()
{}

Response SingleImageLoader::request(const std::string& path)
{
	if (path == _path)
	{
		return _response;
	}

	if (!_loading)
	{
		_loading = true;

		auto ioRunnable = new SingleImageLoadingIORunnable(path);
        connect(ioRunnable, &SingleImageLoadingIORunnable::done, this, &SingleImageLoader::onSingleImageLoadingDone);
        QThreadPool::globalInstance()->start(ioRunnable);
	}

	return Response();
}

void SingleImageLoader::onSingleImageLoadingDone(std::string path, Response response)
{
	_loading = false;

	_path = path;
	_response = response;

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

	// retrieve metadata from disk
    int width, height;
    auto metadata = aliceVision::image::readImageMetadata(_path, width, height);

    // store original image dimensions
    response.dim = QSize(width, height);

    // copy metadata into a QVariantMap
    for (const auto& item : metadata)
    {
        response.metadata[QString::fromStdString(item.name().string())] = QString::fromStdString(item.get_string());
    }

    // load image
    response.img = std::make_shared<aliceVision::image::Image<aliceVision::image::RGBAfColor>>();
    aliceVision::image::readImage(_path, *(response.img), aliceVision::image::EImageColorSpace::LINEAR);

    Q_EMIT done(_path, response);
}

} // namespace imageio
} // namespace qtAliceVision
