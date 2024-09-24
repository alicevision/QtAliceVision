#pragma once

#include "ImageServer.hpp"
#include "ImageCache.hpp"
#include "AsyncFetcher.hpp"

#include <QObject>
#include <QPoint>
#include <QThreadPool>

namespace qtAliceVision {
namespace imgserve {

class SequenceCache : public QObject, public ImageServer
{
    Q_OBJECT

  public:
    // Constructors and destructor

    explicit SequenceCache(QObject* parent = nullptr);

    ~SequenceCache();

  public:
    // Data manipulation for the Qt property system

    /**
     * @brief Set the current image sequence.
     * @param[in] paths unordered list of image filepaths
     * @note the sequence order will not be changed
     */
    void setSequence(const QVariantList& paths);

    /**
     * @brief Set the resize ratio for the images in the sequence.
     * @param[in] ratio target ratio for the image downscale
     */
    void setResizeRatio(double ratio);

    /**
     * @brief Get the frames in the sequence that are currently cached.
     * @return a list of intervals, each one describing a range of cached frames
     * @note we use QPoints to represent the intervals (x: range start, y: range end)
     */
    QVariantList getCachedFrames() const;

    /**
     * @brief Set the boolean flag indicating if the sequence is being fetched asynchroneously.
     * @param[in] fetching new value for the fetching flag
     */
    void setAsyncFetching(bool fetching);

    /**
     * @brief Set the maximum memory that can be filled by the cache.
     * @param[in] memory maximum memory in bytes
     */
    void setMemoryLimit(int memory);

    /**
     * @brief Get the maximum available RAM on the system.
     * @return maximum available RAM in bytes
     */
    QPointF getRamInfo() const;

public:
    // Request management

    /// If the image requested falls outside a certain region of cached images,
    /// this method will launch a worker thread to prefetch new images from disk.
    ResponseData request(const RequestData& reqData) override;

public:
    Q_SLOT void onAsyncFetchProgressed();
    Q_SIGNAL void requestHandled();

private:
    size_t _maxMemory;
    AsyncFetcher _fetcher;
    QThreadPool _threadPool;
};

}  // namespace imgserve
}  // namespace qtAliceVision
