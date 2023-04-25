#pragma once

#include <aliceVision/image/all.hpp>

#include <QSize>
#include <QVariant>
#include <QMap>
#include <QMetaType>

#include <string>
#include <memory>


namespace qtAliceVision {
namespace imgserve {

/**
 * @brief Utility structure to encapsulate the response to an image loading request.
 */
struct Response {

	std::shared_ptr<aliceVision::image::Image<aliceVision::image::RGBAfColor>> img;

	QSize dim;

	QVariantMap metadata;

};

/**
 * @brief Interface for loading images from disk.
 */
class ImageServer {

public:

	/**
	 * @brief Request an image stored on disk with its metadata.
	 * @note this is a pure virtual method
	 * @param[in] path image's filepath on disk
	 * @return a response to the request containing a pointer to the image and the image's metadata
	 */
	virtual Response request(const std::string& path) = 0;

};

} // namespace imgserve
} // namespace qtAliceVision

// Make Response struct known to QMetaType
// for usage in signals and slots
Q_DECLARE_METATYPE(qtAliceVision::imgserve::Response)
