#include "SequenceCache.hpp"

#include <QThreadPool>

#include <algorithm>
#include <cmath>


namespace qtAliceVision {

SequenceCache::SequenceCache(QObject* parent) :
    QObject(parent)
{
    _cache = new aliceVision::image::ImageCache(1024.f, 1024.f, aliceVision::image::EImageColorSpace::LINEAR);
    _extentPrefetch = 10;
    _regionPrefetch = std::make_pair(0, 0);
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

SequenceCache::Response SequenceCache::request(const std::string& path)
{
    // Initialize empty response
    Response response;

    // Check if we are already loading an image
    if (_loading)
    {
        return response;
    }

    // Retrieve frame number corresponding to the requested image in the sequence
    int frame = getFrame(path);
    if (frame < 0)
    {
        return response;
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

    // Update internal state
    _loading = true;

    // Gather images to load
    _regionPrefetch = getRegion(frame, _extentPrefetch);
    std::vector<std::string> toLoad(
        _sequence.begin() + _regionPrefetch.first,
        _sequence.begin() + _regionPrefetch.second);

    // Start prefetching thread
    auto ioRunnable = new PrefetchingIORunnable(_cache, toLoad);
    connect(ioRunnable, &PrefetchingIORunnable::resultReady, this, &SequenceCache::onResultReady);
    QThreadPool::globalInstance()->start(ioRunnable);

    return response;
}

void SequenceCache::onResultReady()
{
    // Update internal state
    _loading = false;

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
