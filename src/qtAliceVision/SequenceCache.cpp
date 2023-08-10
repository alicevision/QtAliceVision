#include "SequenceCache.hpp"

#include <aliceVision/system/MemoryInfo.hpp>

#include <QThreadPool>
#include <QString>
#include <QPoint>

#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <atomic>


namespace qtAliceVision {
namespace imgserve {

// Flag for aborting the prefetching worker thread from the main thread
std::atomic_bool abortPrefetching = false;

SequenceCache::SequenceCache(QObject* parent) :
    QObject(parent)
{
    // Retrieve memory information from system
    const auto memInfo = aliceVision::system::getMemoryInfo();

    // Compute proportion of RAM that can be dedicated to image caching
    // For now we use 30% of available RAM
    const double availableRam = static_cast<double>(memInfo.availableRam);
    const double cacheRatio = 0.3;
    const double cacheRam = cacheRatio * availableRam;

    // Initialize image cache
    const double factorConvertMiB = 1024. * 1024.;
    const float fCacheRam = static_cast<float>(cacheRam / factorConvertMiB);
    _cache = new aliceVision::image::ImageCache(fCacheRam, fCacheRam, aliceVision::image::EImageColorSpace::LINEAR);

    // Initialize internal state
    _regionSafe = std::make_pair(-1, -1);
    _loading = false;
    _targetSize = 1000;
}

SequenceCache::~SequenceCache()
{
    // Free memory occupied by image cache
    if (_cache) delete _cache;
}

void SequenceCache::setSequence(const QVariantList& paths)
{
    // Clear internal state
    _sequence.clear();
    _regionSafe = std::make_pair(-1, -1);

    // Fill sequence vector
    int frameCounter = 0;
    for (const auto& var : paths)
    {
        try
        {
            // Initialize frame data
            FrameData data;
            data.path = var.toString().toStdString();

            // Retrieve metadata from disk
            int width, height;
            auto metadata = aliceVision::image::readImageMetadata(data.path, width, height);

            // Store original image dimensions
            data.dim = QSize(width, height);

            // Copy metadata into a QVariantMap
            for (const auto& item : metadata)
            {
                data.metadata[QString::fromStdString(item.name().string())] = QString::fromStdString(item.get_string());
            }

            // Compute downscale
            const int maxDim = std::max(width, height);
            const int level = static_cast<int>(std::floor(
                std::log2(static_cast<double>(maxDim) / static_cast<double>(_targetSize))));
            data.downscale = 1 << std::max(level, 0);

            // Set frame number
            data.frame = frameCounter;

            // Add to sequence
            _sequence.push_back(data);
            ++frameCounter;
        }
        catch (const std::runtime_error& e)
        {
            // Log error
            std::cerr << e.what() << std::endl;
        }
    }
}

void SequenceCache::setTargetSize(int size)
{
    // Update target size
    _targetSize = size;

    // Update downscale for each frame
    bool refresh = false;
    for (auto& data : _sequence)
    {
        // Compute downscale
        const int maxDim = std::max(data.dim.width(), data.dim.height());
        const int level = static_cast<int>(std::floor(
            std::log2(static_cast<double>(maxDim) / static_cast<double>(_targetSize))));
        const int downscale = 1 << std::max(level, 0);

        refresh = refresh || (data.downscale != downscale);

        data.downscale = downscale;
    }

    if (refresh)
    {
        // Clear internal state
        _regionSafe = std::make_pair(-1, -1);
    }
}

QVariantList SequenceCache::getCachedFrames() const
{
    QVariantList intervals;

    // Accumulator variables
    auto region = std::make_pair(-1, -1);
    bool regionOpen = false;

    // Build cached frames intervals
    for (std::size_t i = 0; i < _sequence.size(); ++i)
    {
        const int frame = static_cast<int>(i);

        // Check if current frame is in cache
        if (_cache->contains<aliceVision::image::RGBAfColor>(_sequence[i].path, _sequence[i].downscale))
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

ResponseData SequenceCache::request(const RequestData& reqData)
{
    // Initialize empty response
    ResponseData response;

    // Retrieve frame number corresponding to the requested image in the sequence
    int frame = getFrame(reqData.path);
    if (frame < 0)
    {
        // Empty response
        return response;
    }

    // Retrieve frame data
    const std::size_t idx = static_cast<std::size_t>(frame);
    const FrameData& data = _sequence[idx];
    
    // Retrieve image from cache
    const bool cachedOnly = true;
    const bool lazyCleaning = false;
    response.img = _cache->get<aliceVision::image::RGBAfColor>(data.path, data.downscale, cachedOnly, lazyCleaning);

    // Retrieve metadata
    response.dim = data.dim;
    response.metadata = data.metadata;

    // Requested image is not in cache
    // and there is already a prefetching thread running
    if (!response.img && _loading)
    {
        // Abort prefetching to avoid waiting until current worker thread is done
        abortPrefetching = true;
    }

    // Request falls outside of safe region
    if ((frame < _regionSafe.first || frame > _regionSafe.second) && !_loading)
    {
        // Make sur abort flag is off before launching a new prefetching thread
        abortPrefetching = false;

        // Update internal state
        _loading = true;

        // Gather images to load
        std::vector<FrameData> toLoad = _sequence;

        // For now fill the allow worker thread to fill the whole cache capacity
        const double fillRatio = 1.;

        // Create new runnable and launch it in worker thread (managed by Qt thread pool)
        auto ioRunnable = new PrefetchingIORunnable(_cache, toLoad, frame, fillRatio);
        connect(ioRunnable, &PrefetchingIORunnable::progressed, this, &SequenceCache::onPrefetchingProgressed);
        connect(ioRunnable, &PrefetchingIORunnable::done, this, &SequenceCache::onPrefetchingDone);
        QThreadPool::globalInstance()->start(ioRunnable);
    }

    return response;
}

void SequenceCache::onPrefetchingProgressed(int)
{
    // Notify listeners that cache content has changed
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
        const std::size_t idx = static_cast<std::size_t>(frame);

        // Grow region on the left as much as possible
        if (_cache->contains<aliceVision::image::RGBAfColor>(_sequence[idx].path, _sequence[idx].downscale))
        {
            regionCached.first = frame;
        }
        else
        {
            break;
        }
    }
    for (int frame = reqFrame; frame < static_cast<int>(_sequence.size()); ++frame)
    {
        const std::size_t idx = static_cast<std::size_t>(frame);

        // Grow region on the right as much as possible
        if (_cache->contains<aliceVision::image::RGBAfColor>(_sequence[idx].path, _sequence[idx].downscale))
        {
            regionCached.second = frame;
        }
        else
        {
            break;
        }
    }

    // Update safe region
    if (regionCached == std::make_pair(-1, -1))
    {
        _regionSafe = std::make_pair(-1, -1);
    }
    else
    {
        // Here we define safe region to cover 80% of cached region
        // The remaining 20% serves to anticipate prefetching
        const int extentCached = (regionCached.second - regionCached.first) / 2;
        const int extentSafe = static_cast<int>(static_cast<double>(extentCached) * 0.8);
        _regionSafe = buildRegion(reqFrame, extentSafe);
    }

    // Notify clients that a request has been handled
    Q_EMIT requestHandled();
}

int SequenceCache::getFrame(const std::string& path) const
{
    // Go through frames until we find a matching filepath
    for (int idx = 0; idx < _sequence.size(); ++idx)
    {
        if (_sequence[idx].path == path)
        {
            return idx;
        }
    }

    // No match found
    return -1;
}

std::pair<int, int> SequenceCache::buildRegion(int frame, int extent) const
{
    // Initialize region equally around central frame
    int start = frame - extent;
    int end = frame + extent;

    // Adjust to sequence bounds
    if (start < 0)
    {
        start = 0;
        end = std::min(static_cast<int>(_sequence.size()) - 1, 2 * extent);
    }
    else if (end >= static_cast<int>(_sequence.size()))
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

    // Timer for sending progress signals
    auto tRef = std::chrono::high_resolution_clock::now();

    // Processing order:
    // Sort frames by distance to request frame
    std::sort(_toLoad.begin(), _toLoad.end(), [this](const FrameData& lhs, const FrameData& rhs) {
        return std::abs(lhs.frame - _reqFrame) < std::abs(rhs.frame - _reqFrame);
    });

    // Accumulator variable to keep track of cache capacity filled with loaded images
    uint64_t filled = 0;

    // Load images from disk to cache
    for (const auto& data : _toLoad)
    {
        // Check if main thread wants to abort prefetching
        if (abortPrefetching)
        {
            abortPrefetching = false;
            Q_EMIT done(_reqFrame);
            return;
        }

        // Check if image size does not exceed limit
        uint64_t memSize =
            static_cast<uint64_t>(data.dim.width() / data.downscale)
            * static_cast<uint64_t>(data.dim.height() / data.downscale)
            * 16;
        if (filled + memSize > _toFill)
        {
            break;
        }

        // Load image in cache
        try
        {
            const bool cachedOnly = false;
            const bool lazyCleaning = false;
            _cache->get<aliceVision::image::RGBAfColor>(data.path, data.downscale, cachedOnly, lazyCleaning);
            filled += memSize;
        }
        catch (const std::runtime_error& e)
        {
            // Log error message
            std::cerr << e.what() << std::endl;
        }

        // Wait a few milliseconds in case another thread needs to query the cache
        std::this_thread::sleep_for(1ms);

        // Regularly send progress signals
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

} // namespace imgserve
} // namespace qtAliceVision

#include "SequenceCache.moc"
