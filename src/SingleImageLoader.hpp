#pragma once

#include "ImageServer.hpp"

#include <QObject>
#include <QRunnable>
#include <QString>

#include <string>


namespace qtAliceVision {
namespace imgserve {

/**
 * @brief Image server that can load a single image at a time.
 */
class SingleImageLoader : public QObject, public ImageServer {

	Q_OBJECT

public:

	// Constructors and destructor

	explicit SingleImageLoader(QObject* parent = nullptr);

	~SingleImageLoader();

public:

	// Request management

	/// If the image requested cannot be retrieved immediatly,
	/// this method will launch a worker thread to load it from disk.
	Response request(const std::string& path) override;

	/**
	 * @brief Slot called when the loading thread is done.
	 * @param[in] path filepath of the loaded image
	 * @param[in] response a Response instance containing the data loaded from disk
	 */
	Q_SLOT void onSingleImageLoadingDone(QString path, Response response);

	/**
	 * @brief Signal emitted when the loading thread is done and a previous request has been handled.
	 */
	Q_SIGNAL void requestHandled();

private:

	// Member variables

	/// Filepath of currently loaded image.
	std::string _path;

	/// Currently loaded image and its metadata.
	Response _response;

	/// Keep track of whether or not there is an active worker thread.
	bool _loading;

};

/**
 * @brief Utility class for loading a single image asynchronously.
 */
class SingleImageLoadingIORunnable : public QObject, public QRunnable {

	Q_OBJECT

public:

	/**
	 * @param[in] path filepath of the image to load
	 */
	explicit SingleImageLoadingIORunnable(const std::string& path);

	~SingleImageLoadingIORunnable();

	/// Main method for loading a single image in a worker thread.
	Q_SLOT void run() override;

	/**
	 * @brief Signal emitted when image loading is finished.
	 * @param[in] path filepath of the loaded image
	 * @param[in] response a Response instance containing the data loaded from disk
	 */
	Q_SIGNAL void done(QString path, Response response);

private:

	/// Filepath of image to load.
	std::string _path;

};

} // namespace imgserve
} // namespace qtAliceVision
