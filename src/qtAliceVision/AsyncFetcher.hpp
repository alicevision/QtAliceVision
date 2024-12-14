#pragma once

#include <QObject>
#include <QRunnable>
#include <QMutex>
#include <QSemaphore>
#include <QVariantList>

#include "ImageCache.hpp"

namespace qtAliceVision {
namespace imgserve {

class AsyncFetcher : public QObject, public QRunnable
{
    Q_OBJECT

  public:
    AsyncFetcher();
    ~AsyncFetcher();

    /**
     * @brief Cache object is created externally.
     * Pass it to the Fetcher for use (Fetcher get ownership)
     * @param cache the cache object to store
     */
    void setCache(ImageCache::uptr&& cache);

    /**
     * @brief set the image sequence
     * The image sequence is a list of image paths which is ordered
     * The Fetcher must not be in asynchronous mode for this function to work
     * As such, the _sequence object is only used in read mode during async mode.
     */
    void setSequence(const std::vector<std::string>& paths);

    /**
     * @brief update the resize ratio of the image
     * @param ratio the coefficient of resize of the loaded images
     */
    void setResizeRatio(double ratio);

    /**
     * @brief Do we enable prefetching ? Means that we are prefetching next frames 
     * in the sequence before asked.
     * @param prefetch true if prefetching is activated 
     */
    void setPrefetching(bool prefetch);

    /**
     * @brief retrieve a frame from the cache in both sync and async mode
     * @param path the image path which should be contained in _sequence.
     * @param image the result image pointer
     * @param metadatas the image metadatas found in the file
     * @param originalWidth the image width before the resize
     * @param originalHeight the image height before the resize
     * @return true if the image was succesfully found in the cache
     */
    bool getFrame(const std::string& path,
                  std::shared_ptr<aliceVision::image::Image<aliceVision::image::RGBAfColor>>& image,
                  oiio::ParamValueList& metadatas,
                  std::size_t& originalWidth,
                  std::size_t& originalHeight);

    /**
     * @brief Internal function for QT to start the asynchronous mode
     */
    Q_SLOT void run() override;

    /**
     * @brief stop asynchronous mode
     * The caller have to wait on the thread pool to guarantee the effective end
     */
    void stopAsync();

    bool isAsync() const { return _isAsynchronous; }

    /**
     * @brief get the cache content size in bytes
     * @return the cache content size in bytes
     */
    std::size_t getCacheSize() const;

    /**
     * @brief get the number of images loaded
     * @return the count of images loaded since the creation of the cache object
     */
    std::size_t getDiskLoads() const;

    /**
     * @brief update maxMemory for the cache
     * @param maxMemory the number of bytes allowed in the cache
     */
    void updateCacheMemory(std::size_t maxMemory);

    /**
     * @brief get a list of regions containing the image frames
     * @return a list of two values (begin, end)
     */
    QVariantList getCachedFrames() const;

  public:
    Q_SIGNAL void onAsyncFetchProgressed();

  private:
    ImageCache::uptr _cache;

    std::vector<std::string> _sequence;
    std::unordered_map<std::string, unsigned> _pathToSeqId;

    QAtomicInt _currentIndex;
    QAtomicInt _isAsynchronous;
    QAtomicInt _isPrefetching;
    QAtomicInt _requestSynchronous;

    double _resizeRatio;
    QMutex _mutexResizeRatio;
    QSemaphore _semLoop;
};

}  // namespace imgserve
}  // namespace qtAliceVision
