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
    const double cacheRatio = 0.5;
    const double cacheRam = cacheRatio * availableRam;

    const double factorConvertMiB = 1024. * 1024.;
    const float fCacheRam = static_cast<float>(cacheRam / factorConvertMiB);
    _cache = new aliceVision::image::ImageCache(fCacheRam, fCacheRam, aliceVision::image::EImageColorSpace::LINEAR);
    std::cout << _cache->toString() << std::endl;

    _regionSafe = std::make_pair(-1, -1);
    _loading = false;
}

SequenceCache::~SequenceCache()
{
    // free memory occupied by image cache
    if (_cache) delete _cache;
}

void SequenceCache::setSequence(const QVariantList& paths)
{
    _sequence.clear();
    _regionSafe = std::make_pair(-1, -1);

    for (const auto& var : paths)
    {
        // initialize frame data
        FrameData data;
        data.path = var.toString().toStdString();

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

    // assign frame numbers
    for (size_t i = 0; i < _sequence.size(); ++i)
    {
        _sequence[i].frame = static_cast<int>(i);
    }
}

QVariantList SequenceCache::getCachedFrames() const
{
    QVariantList cached;

    auto region = std::make_pair(-1, -1);
    bool regionOpen = false;
    for (int frame = 0; frame < _sequence.size(); ++frame)
    {
        if (_cache->contains<aliceVision::image::RGBAfColor>(_sequence[frame].path, 1))
        {
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
            if (regionOpen)
            {
                cached.append(QPoint(region.first, region.second));
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

        // Gather images to load
        std::vector<FrameData> toLoad = _sequence;

        // Start prefetching thread
        const double fillRatio = 0.75;
        auto ioRunnable = new PrefetchingIORunnable(_cache, toLoad, frame, fillRatio);
        connect(ioRunnable, &PrefetchingIORunnable::progressed, this, &SequenceCache::onPrefetchingProgressed);
        connect(ioRunnable, &PrefetchingIORunnable::done, this, &SequenceCache::onPrefetchingDone);
        QThreadPool::globalInstance()->start(ioRunnable);
    }

    // retrieve frame data
    const FrameData& data = _sequence[frame];
    
    // retrieve image from cache
    const bool cachedOnly = true;
    const bool lazyCleaning = false;
    response.img = _cache->get<aliceVision::image::RGBAfColor>(data.path, 1, cachedOnly, lazyCleaning);

    // retrieve metadata
    response.dim = data.dim;
    response.metadata = data.metadata;

    return response;
}

void SequenceCache::onPrefetchingProgressed(int)
{
    Q_EMIT requestHandled();
}

void SequenceCache::onPrefetchingDone(int reqFrame)
{
    // Update internal state
    _loading = false;

    // Retrieve cached region around requested frame
    auto regionCached = std::make_pair(-1, -1);
    for (int frame = reqFrame; frame >= 0; --frame)
    {
        if (_cache->contains<aliceVision::image::RGBAfColor>(_sequence[frame].path, 1))
        {
            regionCached.first = frame;
        }
        else
        {
            break;
        }
    }
    for (int frame = reqFrame; frame < _sequence.size(); ++frame)
    {
        if (_cache->contains<aliceVision::image::RGBAfColor>(_sequence[frame].path, 1))
        {
            regionCached.second = frame;
        }
        else
        {
            break;
        }
    }

    // Update safe region
    const int extentCached = (regionCached.second - regionCached.first) / 2;
    const int extentSafe = static_cast<int>(static_cast<double>(extentCached) * 0.8);
    _regionSafe = getRegion(reqFrame, extentSafe);

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
        end = static_cast<int>(_sequence.size()) - 1;
        start = std::max(0, static_cast<int>(_sequence.size()) - 1 - 2 * extent);
    }

    return std::make_pair(start, end);
}

PrefetchingIORunnable::PrefetchingIORunnable(aliceVision::image::ImageCache* cache,
                                             const std::vector<FrameData>& toLoad,
                                             int reqFrame,
                                             double fillRatio) :
    _cache(cache), _toLoad(toLoad), _reqFrame(reqFrame)
{
    _toFill = static_cast<uint64_t>(static_cast<double>(_cache->info().capacity) * fillRatio);
}

PrefetchingIORunnable::~PrefetchingIORunnable()
{}

void PrefetchingIORunnable::run()
{
    using namespace std::chrono_literals;

    // timer for sending progress signals
    auto tRef = std::chrono::high_resolution_clock::now();

    // processing order:
    // sort frames by distance to request frame
    std::sort(_toLoad.begin(), _toLoad.end(), [this](const FrameData& lhs, const FrameData& rhs) {
        return std::abs(lhs.frame - _reqFrame) < std::abs(rhs.frame - _reqFrame);
    });

    // accumulator variable to keep track of cache capacity filled with loaded images
    uint64_t filled = 0;

    // Load images from disk to cache
    for (const auto& data : _toLoad)
    {
        // checked if image size does not exceed limit
        uint64_t memSize = static_cast<uint64_t>(data.dim.width()) * static_cast<uint64_t>(data.dim.height()) * 16;
        if (filled + memSize > _toFill)
        {
            break;
        }

        // load image
        const bool cachedOnly = false;
        const bool lazyCleaning = false;
        _cache->get<aliceVision::image::RGBAfColor>(data.path, 1, cachedOnly, lazyCleaning);
        filled += memSize;

        // wait a few milliseconds in case another thread needs to query the cache
        std::this_thread::sleep_for(1ms);

        // regularly send progress signals
        auto tNow = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = tNow - tRef;
        if (diff.count() > 1.)
        {
            tRef = tNow;
            Q_EMIT progressed(_reqFrame);
        }
    }

    // Notify main thread that loading is done
    Q_EMIT done(_reqFrame);
}

} // namespace qtAliceVision

#include "SequenceCache.moc"
