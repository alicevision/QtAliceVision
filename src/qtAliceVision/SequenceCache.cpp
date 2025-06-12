#include "SequenceCache.hpp"

#include <aliceVision/system/MemoryInfo.hpp>

using namespace aliceVision;

namespace qtAliceVision {
namespace imgserve {

SequenceCache::SequenceCache(QObject* parent)
  : QObject(parent)
{
    // Retrieve memory information from system
    const auto memInfo = system::getMemoryInfo();

    // Compute proportion of RAM that can be dedicated to image caching
    // For now we use 30% of available RAM
    const double availableRam = static_cast<double>(memInfo.availableRam);
    const double cacheRatio = 0.3;
    const double cacheRam = cacheRatio * availableRam;

    _maxMemory = static_cast<size_t>(cacheRam);

    _fetcher.setAutoDelete(false);

    // Cache does not exist
    // Let's create a new one!
    {
        ImageCache::uptr cache = std::make_unique<ImageCache>(_maxMemory, image::EImageColorSpace::LINEAR);
        _fetcher.setCache(std::move(cache));
    }
}

SequenceCache::~SequenceCache()
{
    _fetcher.stopAsync();
    _threadPool.waitForDone();
}

void SequenceCache::setSequence(const QVariantList& paths)
{
    _fetcher.stopAsync();
    _threadPool.waitForDone();

    // Convert to string
    std::vector<std::string> sequence;
    for (const auto& item : paths)
    {
        sequence.push_back(item.toString().toStdString());
    }

    // Assign sequence to fetcher
    _fetcher.setSequence(sequence);

    // Restart if needed
    const bool isAsync = true;
    setAsyncFetching(isAsync);
}

void SequenceCache::setResizeRatio(double ratio) 
{ 
    _fetcher.setResizeRatio(ratio); 
}

std::size_t SequenceCache::getMemoryLimit()
{
    // Convert parameter in gigabytes to bytes
    const double gigaBytesToBytes = 1024. * 1024. * 1024.;
    const size_t memory = _fetcher.getCacheMemory();
    return static_cast<std::size_t>(static_cast<double>(memory) / gigaBytesToBytes);
}

void SequenceCache::setMemoryLimit(std::size_t memory)
{
    // Convert parameter in gigabytes to bytes
    const double gigaBytesToBytes = 1024. * 1024. * 1024.;
    _maxMemory = static_cast<std::size_t>(static_cast<double>(memory) * gigaBytesToBytes);
    _fetcher.updateCacheMemory(_maxMemory);
}

QVariantList SequenceCache::getCachedFrames() const { return _fetcher.getCachedFrames(); }

void SequenceCache::setAsyncFetching(bool fetching)
{
    // Always stop first
    _fetcher.stopAsync();
    _threadPool.waitForDone();

    if (fetching)
    {
        connect(&_fetcher, &AsyncFetcher::onAsyncFetchProgressed, this, &SequenceCache::onAsyncFetchProgressed);
        _threadPool.start(&_fetcher);
    }
}

void SequenceCache::setPrefetching(bool prefetching) 
{ 
    _fetcher.setPrefetching(prefetching); 
}

bool SequenceCache::getPrefetching()
{
    return _fetcher.getPrefetching();
}

QPointF SequenceCache::getRamInfo() const
{
    // Get available RAM in bytes and cache occupied memory
    const auto memInfo = aliceVision::system::getMemoryInfo();

    double availableRam = static_cast<double>(memInfo.availableRam) / (1024. * 1024. * 1024.);
    double contentSize = static_cast<double>(_fetcher.getCacheSize()) / (1024. * 1024. * 1024. * 1024.);

    // Return in GB
    return QPointF(availableRam, contentSize);
}

ResponseData SequenceCache::request(const RequestData& reqData)
{
    // Initialize empty response
    ResponseData response;

    std::shared_ptr<image::Image<image::RGBAfColor>> image;
    oiio::ParamValueList metadatas;
    size_t originalWidth = 0;
    size_t originalHeight = 0;
    bool hadErrorOnLoad = false;
    bool isFileMissing = false;

    if (!_fetcher.getFrame(reqData.path, image, metadatas, originalWidth, originalHeight, isFileMissing, hadErrorOnLoad))
    {
        return response;
    }

    if (isFileMissing)
    {
        response.error = MISSING_FILE;
        return response;
    }

    if (hadErrorOnLoad)
    {
        response.error = LOADING_ERROR;
        return response;
    }

    // Build a new response with information fetched
    response.metadata.clear();
    response.img = image;
    response.dim = QSize(static_cast<int>(originalWidth), static_cast<int>(originalHeight));

    // Convert metadatas
    for (const auto& item : metadatas)
    {
        response.metadata[QString::fromStdString(item.name().string())] = QString::fromStdString(item.get_string());
    }

    return response;
}

void SequenceCache::onAsyncFetchProgressed()
{
    // Notify listeners that cache content has changed
    Q_EMIT requestHandled();
}

}  // namespace imgserve
}  // namespace qtAliceVision

#include "SequenceCache.moc"
