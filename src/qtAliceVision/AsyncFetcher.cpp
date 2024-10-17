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
    _isAsynchroneous = false;
    _requestSynchroneous = false;
}

AsyncFetcher::~AsyncFetcher()
{
    
}

void AsyncFetcher::setSequence(const std::vector<std::string> & paths)
{
    //Sequence can't be changed while thread is running
    if (_isAsynchroneous)
    {
        return;
    } 

    _sequence = paths;

    for (unsigned idx = 0; idx < _sequence.size(); idx++)
    {
        _pathToSeqId[_sequence[idx]] = idx;
    }

    _currentIndex = 0;
}

void AsyncFetcher::setResizeRatio(double ratio)
{
    QMutexLocker locker(&_mutexResizeRatio);
    _resizeRatio = ratio;
}

void AsyncFetcher::setCache(aliceVision::image::ImageCache::uptr && cache)
{
    //Cache can't be changed while thread is running
    if (_isAsynchroneous)
    {
        return;
    } 
    _cache = std::move(cache);
}


void AsyncFetcher::run()
{
    using namespace std::chrono_literals;

    _isAsynchroneous = true;
    _requestSynchroneous = false;

    int previousCacheSize = getDiskLoads();

    while (1)
    {
        if (_requestSynchroneous)
        {
            _requestSynchroneous = false;
            break;
        }

        {
            const std::string & lpath = _sequence[_currentIndex];

            //Load in cache
            if (_cache)
            {
                double ratio;
                {
                    QMutexLocker locker(&_mutexResizeRatio);
                    ratio = _resizeRatio;
                }
                
                _cache->get<image::RGBAfColor>(lpath, _currentIndex, ratio, false);
            }

            _currentIndex++;

            int size = _sequence.size();
            if (_currentIndex >= size)
            {
                _currentIndex = 0;
            }
        }

        std::this_thread::sleep_for(1ms);

        int cacheSize = getDiskLoads();
        if (cacheSize != previousCacheSize)
        {
            previousCacheSize = cacheSize;
            Q_EMIT onAsyncFetchProgressed();
        }
    }

    _requestSynchroneous = false;
    _isAsynchroneous = false;
}

void AsyncFetcher::stopAsync()
{    
    _requestSynchroneous = true;
}

void AsyncFetcher::updateCacheMemory(size_t maxMemory)
{
    if (_cache)
    {
        _cache->updateMaxMemory(maxMemory);
    }
}

size_t AsyncFetcher::getCacheSize() const
{
    return (_cache)?_cache->info().getContentSize():0.0f;
}

size_t AsyncFetcher::getDiskLoads() const
{
    return (_cache)?_cache->info().getLoadFromDisk():0.0f;
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

bool AsyncFetcher::getFrame(const std::string & path, 
                std::shared_ptr<image::Image<image::RGBAfColor>> & image, 
                oiio::ParamValueList & metadatas,
                size_t & originalWidth,
                size_t & originalHeight)
{
    //Need a cache
    if (!_cache)
    {
        return false;
    }

    // First try getting the image
    bool onlyCache = _isAsynchroneous;
          
    //Upgrade the thread with the current Index
    for (int idx = 0; idx < _sequence.size(); ++idx)
    {
        if (_sequence[idx] == path)
        {
            _currentIndex = idx;
            break;
        }
    }

    std::optional<image::CacheValue> ovalue = _cache->get<image::RGBAfColor>(path, _currentIndex, _resizeRatio, onlyCache);
        
        
    if (ovalue.has_value())
    {
        auto & value = ovalue.value();
        image = value.get<image::RGBAfColor>();

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

}
}

#include "AsyncFetcher.moc"