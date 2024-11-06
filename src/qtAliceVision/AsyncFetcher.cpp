#include "AsyncFetcher.hpp"

#include <QMutexLocker>
#include <QPoint>

#include <thread>
#include <chrono>

using namespace aliceVision;

namespace qtAliceVision {
namespace imgserve {

AsyncFetcher::AsyncFetcher()
{
    _resizeRatio = 0.001;
    _isAsynchronous = false;
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

    for (unsigned idx = 0; idx < _sequence.size(); idx++)
    {
        _pathToSeqId[_sequence[idx]] = idx;
    }
}

void AsyncFetcher::setResizeRatio(double ratio)
{
    QMutexLocker locker(&_mutexResizeRatio);
    _resizeRatio = ratio;
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

        if (_sequence.size() == 0)
        {
            std::this_thread::sleep_for(100ms);
        }
        else
        {
            const std::string& lpath = _sequence[static_cast<std::size_t>(_currentIndex)];

            // Load in cache
            if (_cache)
            {
                double ratio;
                {
                    QMutexLocker locker(&_mutexResizeRatio);
                    ratio = _resizeRatio;
                }

                _cache->get<image::RGBAfColor>(lpath, static_cast<unsigned int>(_currentIndex), ratio, false);
            }

            _currentIndex++;

            int size = static_cast<int>(_sequence.size());
            if (_currentIndex >= size)
            {
                _currentIndex = 0;
            }

            std::this_thread::sleep_for(1ms);
        }

        std::size_t cacheSize = getDiskLoads();
        if (cacheSize != previousCacheSize)
        {
            previousCacheSize = cacheSize;
            Q_EMIT onAsyncFetchProgressed();
        }
    }

    _requestSynchronous = false;
    _isAsynchronous = false;
}

void AsyncFetcher::stopAsync()
{
    _requestSynchronous = true;
}

void AsyncFetcher::updateCacheMemory(std::size_t maxMemory)
{
    if (_cache)
    {
        _cache->updateMaxMemory(maxMemory);
    }
}

std::size_t AsyncFetcher::getCacheSize() const
{
    return (_cache) ? static_cast<std::size_t>(_cache->info().getContentSize()) : 0;
}

std::size_t AsyncFetcher::getDiskLoads() const
{
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

    {
        // Build cached frames intervals
        for (std::size_t i = 0; i < size; ++i)
        {
            const int frame = static_cast<int>(i);

            // Check if current frame is in cache
            if (_cache->contains<aliceVision::image::RGBAfColor>(_sequence[i], _resizeRatio))
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
    }

    // Last region may still be open
    if (regionOpen)
    {
        intervals.append(QPoint(region.first, region.second));
    }

    return intervals;
}

bool AsyncFetcher::getFrame(const std::string& path,
                            std::shared_ptr<image::Image<image::RGBAfColor>>& image,
                            oiio::ParamValueList& metadatas,
                            size_t& originalWidth,
                            size_t& originalHeight)
{
    // Need a cache
    if (!_cache)
    {
        return false;
    }

    // First try getting the image
    bool onlyCache = _isAsynchronous;

    // Upgrade the thread with the current Index
    for (std::size_t idx = 0; idx < _sequence.size(); ++idx)
    {
        if (_sequence[idx] == path)
        {
            _currentIndex = static_cast<int>(idx);
            break;
        }
    }

    std::optional<CacheValue> ovalue = _cache->get<aliceVision::image::RGBAfColor>(path, _currentIndex, _resizeRatio, onlyCache);

    if (ovalue.has_value())
    {
        auto& value = ovalue.value();
        image = value.get<aliceVision::image::RGBAfColor>();

        oiio::ParamValueList copy_metadatas = value.getMetadatas();
        metadatas = copy_metadatas;
        originalWidth = value.getOriginalWidth();
        originalHeight = value.getOriginalHeight();

        if (image)
        {
            _cache->setReferenceFrameId(_currentIndex);
        }

        return true;
    }

    return false;
}

}  // namespace imgserve
}  // namespace qtAliceVision

#include "AsyncFetcher.moc"
