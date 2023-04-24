#include "SequenceCache.hpp"

#include <QThreadPool>

#include <algorithm>
#include <cmath>


namespace qtAliceVision {

SequenceCache::SequenceCache(QObject* parent) :
    QObject(parent)
{
    _cache = new aliceVision::image::ImageCache(1024.f, 1024.f, aliceVision::image::EImageColorSpace::LINEAR);
    _extentPrefetch = 30;
    _regionPrefetch = std::make_pair(0, 0);
    _extentSafe = 20;
    _regionSafe = std::make_pair(0, 0);
    _loading = false;
}

SequenceCache::~SequenceCache()
{
    if (_cache) delete _cache;
}

void SequenceCache::setSequence(const std::vector<std::string>& sequence)
{
    _sequence = sequence;
    std::sort(_sequence.begin(), _sequence.end());
}

std::vector<int> SequenceCache::getCachedFrames() const
{
    std::vector<int> cached;

    for (int frame = 0; frame < _sequence.size(); ++frame)
    {
        if (_cache->contains<aliceVision::image::RGBAfColor>(_sequence[frame].path, _sequence[frame].downscale))
        {
            cached.push_back(frame);
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
        std::vector<std::string> toLoad(
            _sequence.begin() + _nextRegionPrefetch.first,
            _sequence.begin() + _nextRegionPrefetch.second);

        // Start prefetching thread
        auto ioRunnable = new PrefetchingIORunnable(_cache, toLoad, _maxResolution);
        connect(ioRunnable, &PrefetchingIORunnable::resultReady, this, &SequenceCache::onResultReady);
        QThreadPool::globalInstance()->start(ioRunnable);
    }

    // Image is in prefetching region, therefore it must already be cached
    if (frame >= _regionPrefetch.first && frame <= _regionPrefetch.second)
    {
        // retrieve image from cache
        response.img = _cache->get<aliceVision::image::RGBAfColor>(path);

        // load metadata and image dimensions
        int width, height;
        const auto metadata = aliceVision::image::readImageMetadata(path, width, height);

        response.dim = QSize(width, height);
        
        for (const auto& item : metadata)
        {
            response.metadata[QString::fromStdString(item.name().string())] = QString::fromStdString(item.get_string());
        }

        return response;
    }
    
    return response;
}

void SequenceCache::onResultReady()
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
        if (_sequence[idx] == path)
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

PrefetchingIORunnable::PrefetchingIORunnable(aliceVision::image::ImageCache* cache,
                                             const std::vector<std::string>& toLoad) :
    _cache(cache), _toLoad(toLoad)
{}

PrefetchingIORunnable::~PrefetchingIORunnable()
{}

void PrefetchingIORunnable::run()
{
    // Load images from disk to cache
    for (const auto& path : _toLoad)
    {
        _cache->get<aliceVision::image::RGBAfColor>(path);
    }

    // Notify main thread that loading is done
    Q_EMIT resultReady();
}

} // namespace qtAliceVision

#include "SequenceCache.moc"
