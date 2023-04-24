#pragma once

#include <aliceVision/image/all.hpp>

#include <QSize>
#include <QVariant>
#include <QMap>
#include <QMetaType>

#include <string>
#include <memory>


namespace qtAliceVision {
namespace imageio {

/**
 * @brief Utility structure to encapsulate the response to an image request.
 */
struct Response {

	std::shared_ptr<aliceVision::image::Image<aliceVision::image::RGBAfColor>> img;

	QSize dim;

	QVariantMap metadata;

};

/**
 * @brief Interface for image server classes.
 */
class ImageServer {

public:

	virtual Response request(const std::string& path) = 0;

};

Q_DECLARE_METATYPE(Response)

} // namespace imageio
} // namespace qtAliceVision
