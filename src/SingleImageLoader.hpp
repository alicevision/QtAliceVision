#pragma once

#include "ImageServer.hpp"

#include <QObject>
#include <QRunnable>

#include <string>


namespace qtAliceVision {
namespace imageio {

/**
 * @brief TODO
 */
class SingleImageLoader : public QObject, public ImageServer {

	Q_OBJECT

public:

	explicit SingleImageLoader(QObject* parent = nullptr);

	~SingleImageLoader();

public:

	// Request management

	Response request(const std::string& path) override;

	Q_SLOT void onSingleImageLoadingDone(Response response);

	Q_SIGNAL void requestHandled();

private:

	std::string _path;

	Response _response;

	bool _loading;
	std::string _nextPath;

};

class SingleImageLoadingIORunnable : public QObject, public QRunnable {

	Q_OBJECT

public:

	SingleImageLoadingIORunnable(const std::string& path);

	~SingleImageLoadingIORunnable();

	Q_SLOT void run() override;

	Q_SIGNAL void done(Response response);

private:

	std::string _path;

};

} // namespace imageio
} // namespace qtAliceVision
