#include "AsyncFetcher.hpp"

#include <QMutexLocker>
#include <QPoint>

#include <optional>
#include <thread>
#include <chrono>

namespace qtAliceVision {
namespace imgserve {

AsyncFetcher::AsyncFetcher()
{
    _resizeRatio = 0.001;
    _isAsynchronous = false;
    _isPrefetching = false;
    _requestSynchronous = false;
}

AsyncFetcher::~AsyncFetcher() {}

void AsyncFetcher::setSequence(const std::vector<std::string>& paths)
{
    // Sequence can't be changed while thread is running
    if (_isAsynchronous)
    {
        return;
    }

    _sequence = paths;
    _currentIndex = 0;

    for (std::size_t idx = 0; idx < _sequence.size(); ++idx)
    {
        _pathToSeqId[_sequence[idx]] = static_cast<unsigned>(idx);
    }
}

void AsyncFetcher::setResizeRatio(double ratio)
{
    if (ratio <= 0.0)
    {
        return;
    }
    QMutexLocker locker(&_mutexResizeRatio);
    _resizeRatio = ratio;
}

void AsyncFetcher::setPrefetching(bool prefetch)
{
    _isPrefetching = prefetch;

    if (_isPrefetching)
    {
        // Make sure we're not waiting for new source
        _semLoop.release(1);
    }
}

bool AsyncFetcher::getPrefetching() const
{
    return _isPrefetching;
}

void AsyncFetcher::setCache(ImageCache::uptr&& cache)
{
    // Cache can't be changed while thread is running
    if (_isAsynchronous)
    {
        return;
    }
    _cache = std::move(cache);
}

void AsyncFetcher::run()
{
    using namespace std::chrono_literals;

    _isAsynchronous = true;
    _requestSynchronous = false;

    std::size_t previousCacheSize = getDiskLoads();

    while (1)
    {
        if (_requestSynchronous)
        {
            _requestSynchronous = false;
            break;
        }

        // Lock the thread until someone ask something
        if (!_semLoop.tryAcquire(1, QDeadlineTimer(1s)))
        {
            continue;
        }

        if (_sequence.empty())
        {
            continue;
        }

        const std::string& lpath = _sequence[static_cast<std::size_t>(_currentIndex)];

        // Load in cache
        if (_cache)
        {
            double ratio;
            {
                QMutexLocker locker(&_mutexResizeRatio);
                ratio = _resizeRatio;
            }

            _cache->get<aliceVision::image::RGBAfColor>(lpath, static_cast<unsigned int>(_currentIndex), ratio, false);
        }

        if (_isPrefetching)
        {
            _currentIndex++;

            int size = static_cast<int>(_sequence.size());
            if (_currentIndex >= size)
            {
                _currentIndex = 0;
            }

            _semLoop.release(1);
        }

        std::this_thread::sleep_for(1ms);

        std::size_t cacheSize = getDiskLoads();
        if (cacheSize != previousCacheSize)
        {
            previousCacheSize = cacheSize;
            Q_EMIT onAsyncFetchProgressed();
        }
    }

    _isAsynchronous = false;
}

void AsyncFetcher::stopAsync() { _requestSynchronous = true; }

void AsyncFetcher::updateCacheMemory(std::size_t maxMemory)
{
    if (_cache)
    {
        _cache->updateMaxMemory(maxMemory);
    }
}

std::size_t AsyncFetcher::getCacheMemory() const
{
    return (_cache) ? _cache->getMaxMemory() : 0;
}

std::size_t AsyncFetcher::getCacheSize() const 
{
    return (_cache) ? static_cast<std::size_t>(_cache->info().getContentSize()) : 0; 
}

std::size_t AsyncFetcher::getDiskLoads() const { 
    return (_cache) ? static_cast<std::size_t>(_cache->info().getLoadFromDisk()) : 0; 
}

QVariantList AsyncFetcher::getCachedFrames() const
{
    QVariantList intervals;

    if (!_cache)
    {
        return intervals;
    }

    // Accumulator variables
    auto region = std::make_pair(-1, -1);
    bool regionOpen = false;

    size_t size = _sequence.size();

    double resizeRatio;
    {
        QMutexLocker locker(&_mutexResizeRatio);
        resizeRatio = _resizeRatio;
    }

    // Build cached frames intervals
    for (std::size_t i = 0; i < size; ++i)
    {
        const int frame = static_cast<int>(i);

        // Check if current frame is in cache
        if (_cache->contains<aliceVision::image::RGBAfColor>(_sequence[i], resizeRatio))
        {
            // Either grow currently open region or create a new region
            if (regionOpen)
            {
                region.second = frame;
            }
            else
            {
                region.first = frame;
                region.second = frame;
                regionOpen = true;
            }
        }
        else
        {
            // Close currently open region
            if (regionOpen)
            {
                intervals.append(QPoint(region.first, region.second));
                regionOpen = false;
            }
        }
    }

    // Last region may still be open
    if (regionOpen)
    {
        intervals.append(QPoint(region.first, region.second));
    }

    return intervals;
}

bool AsyncFetcher::getFrame(const std::string& path,
                            std::shared_ptr<aliceVision::image::Image<aliceVision::image::RGBAfColor>>& image,
                            oiio::ParamValueList& metadatas,
                            size_t& originalWidth,
                            size_t& originalHeight,
                            bool& missingFile,
                            bool& loadingError)
{
    // Need a cache
    if (!_cache)
    {
        return false;
    }

    // Do we only lookup in the cache or do we allow to load on disk immediately
    bool onlyCache = _isAsynchronous;

    // Upgrade the thread with the current Index
    auto it = _pathToSeqId.find(path);
    if (it != _pathToSeqId.end())
    {
        _currentIndex = static_cast<int>(it->second);
    }

    // Try to find in the cache
    double ratio;
    {
        QMutexLocker locker(&_mutexResizeRatio);
        ratio = _resizeRatio;
    }
    std::optional<CacheValue> ovalue = _cache->get<aliceVision::image::RGBAfColor>(path, static_cast<unsigned int>(_currentIndex), ratio, onlyCache);

    if (ovalue.has_value())
    {
        auto& value = ovalue.value();
        image = value.get<aliceVision::image::RGBAfColor>();

        metadatas = value.getMetadatas();
        originalWidth = value.getOriginalWidth();
        originalHeight = value.getOriginalHeight();
        missingFile = value.isFileMissing();
        loadingError = value.hadErrorOnLoad();

        if (image)
        {
            _cache->setReferenceFrameId(_currentIndex);
        }

        return true;
    }
    else
    {
        // Image not yet in cache; poke the fetch thread to load it
        _semLoop.release(1);
    }

    return false;
}

}  // namespace imgserve
}  // namespace qtAliceVision
