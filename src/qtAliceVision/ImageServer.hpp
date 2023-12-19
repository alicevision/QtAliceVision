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

enum LoadingStatus
{
    SUCCESSFUL,
    MISSING_FILE,
    ERROR
};

/**
 * @brief Utility structure to encapsulate an image loading request.
 */
struct RequestData
{
    std::string path;

    int downscale = 1;
};

/**
 * @brief Utility structure to encapsulate the response to an image loading request.
 */
struct ResponseData
{
    std::shared_ptr<aliceVision::image::Image<aliceVision::image::RGBAfColor>> img;

    QSize dim;

    QVariantMap metadata;

    LoadingStatus error = SUCCESSFUL;
};

/**
 * @brief Interface for loading images from disk.
 */
class ImageServer
{
  public:
    /**
     * @brief Request an image stored on disk with its metadata.
     * @note this is a pure virtual method
     * @param[in] path image's filepath on disk
     * @return a response to the request containing a pointer to the image and the image's metadata
     */
    virtual ResponseData request(const RequestData& reqData) = 0;
};

}  // namespace imgserve
}  // namespace qtAliceVision

// Make RequestData and ResponseData struct known to QMetaType
// for usage in signals and slots
Q_DECLARE_METATYPE(qtAliceVision::imgserve::RequestData)
Q_DECLARE_METATYPE(qtAliceVision::imgserve::ResponseData)
