#include "SequenceCache.hpp"

#include <aliceVision/system/MemoryInfo.hpp>

#include <QThreadPool>
#include <QPoint>

#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include <iostream>


namespace qtAliceVision {

SequenceCache::SequenceCache(QObject* parent) :
    QObject(parent)
{
    const auto memInfo = aliceVision::system::getMemoryInfo();
    std::cout << memInfo << std::endl;

    const double availableRam = static_cast<double>(memInfo.availableRam);
    const double cacheRatio = 0.25;
    const double cacheRam = cacheRatio * availableRam;

    const double factorConvertMiB = 1024. * 1024.;
    const float fCacheRam = static_cast<float>(cacheRam / factorConvertMiB);
    _cache = new aliceVision::image::ImageCache(fCacheRam, fCacheRam, aliceVision::image::EImageColorSpace::LINEAR);
    std::cout << _cache->toString() << std::endl;

    _extentPrefetch = 30;
    _extentSafe = 20;
    clearRegions();

    _loading = false;
}

SequenceCache::~SequenceCache()
{
    // free memory occupied by image cache
    if (_cache) delete _cache;
}

void SequenceCache::setSequence(const std::vector<std::string>& paths)
{
    _sequence.clear();

    for (size_t i = 0; i < paths.size(); ++i)
    {
        // initialize frame data
        FrameData data;
        data.path = paths[i];

        // retrieve metadata from disk
        int width, height;
        auto metadata = aliceVision::image::readImageMetadata(data.path, width, height);

        // store original image dimensions
        data.dim = QSize(width, height);

        // copy metadata into a QVariantMap
        for (const auto& item : metadata)
        {
            data.metadata[QString::fromStdString(item.name().string())] = QString::fromStdString(item.get_string());
        }

        _sequence.push_back(data);
    }

    // sort sequence by filepaths
    std::sort(_sequence.begin(), _sequence.end(), [](const FrameData& d1, const FrameData& d2) {
        return d1.path < d2.path;
    });

    clearRegions();
}

std::vector<int> SequenceCache::getCachedFrames() const
{
    std::vector<int> cached;

    int regionStart, regionEnd;
    bool regionOpen = false;
    for (int frame = 0; frame < _sequence.size(); ++frame)
    {
        if (_cache->contains<aliceVision::image::RGBAfColor>(_sequence[frame].path, 1))
        {
            if (regionOpen)
            {
                regionEnd = frame;
            }
            else
            {
                regionStart = frame;
                regionEnd = frame;
                regionOpen = true;
            }
        }
        else
        {
            if (regionOpen)
            {
                cached.append(QPoint(regionStart, regionEnd));
                regionOpen = false;
            }
        }
    }

    return cached;
}

SequenceCache::Response SequenceCache::request(const std::string& path)
{
    // Initialize empty response
    Response response;

    // Retrieve frame number corresponding to the requested image in the sequence
    int frame = getFrame(path);
    if (frame < 0)
    {
        return response;
    }

    // Request falls outside of safe region
    if ((frame < _regionSafe.first || frame > _regionSafe.second) && !_loading)
    {
        // Update internal state
        _loading = true;
        _nextRegionPrefetch = getRegion(frame, _extentPrefetch);
        _nextRegionSafe = getRegion(frame, _extentSafe);

        // Gather images to load
        std::vector<FrameData> toLoad(
            _sequence.begin() + _nextRegionPrefetch.first,
            _sequence.begin() + _nextRegionPrefetch.second);

        // Start prefetching thread
        auto ioRunnable = new PrefetchingIORunnable(_cache, toLoad);
        connect(ioRunnable, &PrefetchingIORunnable::progressed, this, &SequenceCache::onPrefetchingProgressed);
        connect(ioRunnable, &PrefetchingIORunnable::done, this, &SequenceCache::onPrefetchingDone);
        QThreadPool::globalInstance()->start(ioRunnable);
    }

    // Image is in prefetching region, therefore it must already be cached
    if (frame >= _regionPrefetch.first && frame <= _regionPrefetch.second)
    {
        // retrieve frame data
        const FrameData& data = _sequence[frame];
        
        // retrieve image from cache
        response.img = _cache->get<aliceVision::image::RGBAfColor>(data.path, 1);

        // retrieve metadata
        response.dim = data.dim;
        response.metadata = data.metadata;

        return response;
    }
    
    return response;
}

void SequenceCache::onPrefetchingProgressed()
{
    Q_EMIT requestHandled();
}

void SequenceCache::onPrefetchingDone()
{
    // Update internal state
    _loading = false;
    _regionPrefetch = _nextRegionPrefetch;
    _regionSafe = _nextRegionSafe;

    // Notify clients that a request has been handled
    Q_EMIT requestHandled();
}

int SequenceCache::getFrame(const std::string& path) const
{
    for (int idx = 0; idx < _sequence.size(); ++idx)
    {
        if (_sequence[idx].path == path)
        {
            return idx;
        }
    }
    return -1;
}

std::pair<int, int> SequenceCache::getRegion(int frame, int extent) const
{
    int start = frame - extent;
    int end = frame + extent;

    if (start < 0)
    {
        start = 0;
        end = std::min(static_cast<int>(_sequence.size()) - 1, 2 * extent);
    }
    else if (end >= _sequence.size())
    {
        end = _sequence.size() - 1;
        start = std::max(0, static_cast<int>(_sequence.size()) - 1 - 2 * extent);
    }

    return std::make_pair(start, end);
}

void SequenceCache::clearRegions()
{
    _regionPrefetch = std::make_pair(-1, -1);
    _regionSafe = std::make_pair(-1, -1);
}

PrefetchingIORunnable::PrefetchingIORunnable(aliceVision::image::ImageCache* cache,
                                             const std::vector<FrameData>& toLoad) :
    _cache(cache), _toLoad(toLoad)
{}

PrefetchingIORunnable::~PrefetchingIORunnable()
{}

void PrefetchingIORunnable::run()
{
    using namespace std::chrono_literals;

    auto tRef = std::chrono::high_resolution_clock::now();

    // Load images from disk to cache
    for (const auto& data : _toLoad)
    {
        // load image
        _cache->get<aliceVision::image::RGBAfColor>(data.path, 1);

        // wait a few milliseconds in case another thread needs to query the cache
        std::this_thread::sleep_for(1ms);

        // regularly send progress signals
        auto tNow = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = tNow - tRef;
        if (diff.count() > 1.)
        {
            tRef = tNow;
            Q_EMIT progressed();
        }
    }

    // Notify main thread that loading is done
    Q_EMIT done();
}

} // namespace qtAliceVision

#include "SequenceCache.moc"
