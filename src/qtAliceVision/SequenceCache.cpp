#include "SequenceCache.hpp"

#include <aliceVision/system/MemoryInfo.hpp>
#include <aliceVision/image/Image.hpp>

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
    
    //Cache does not exist
    //Let's create a new one !
    {
        image::ImageCache::uptr cache = std::make_unique<image::ImageCache>(_maxMemory, image::EImageColorSpace::LINEAR);
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
    bool isAsync = _fetcher.isAsync();

    _fetcher.stopAsync();
    _threadPool.waitForDone();

    //Convert to string
    std::vector<std::string> sequence;
    for (const auto & item : paths)
    {
        sequence.push_back(item.toString().toStdString());
    }

    //Assign sequence to fetcher
    _fetcher.setSequence(sequence);

    //Restart if needed
    setAsyncFetching(isAsync);
}

void SequenceCache::setResizeRatio(double ratio)
{
    _fetcher.setResizeRatio(ratio);
}

void SequenceCache::setMemoryLimit(int memory)
{
    // convert parameter in gigabytes to bytes
    
    const double gigaBytesToBytes = 1024. * 1024. * 1024.;
    _maxMemory = static_cast<double>(memory) * gigaBytesToBytes;
    _fetcher.updateCacheMemory(_maxMemory);
}

QVariantList SequenceCache::getCachedFrames() const
{
    return _fetcher.getCachedFrames();
}

void SequenceCache::setAsyncFetching(bool fetching)
{    
    //Always stop first
    _fetcher.stopAsync();
    _threadPool.waitForDone();
    
    if (fetching)
    {        
        connect(&_fetcher, &AsyncFetcher::onAsyncFetchProgressed, this, &SequenceCache::onAsyncFetchProgressed);
        _threadPool.start(&_fetcher);
    }
}

QPointF SequenceCache::getRamInfo() const
{
    // get available RAM in bytes and cache occupied memory
    const auto memInfo = aliceVision::system::getMemoryInfo();

    double availableRam = memInfo.availableRam / (1024. * 1024. * 1024.);
    double contentSize = static_cast<double>(_fetcher.getCacheSize()) / (1024. * 1024. * 1024. * 1024.);

    // return in GB
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

    if (!_fetcher.getFrame(reqData.path, image, metadatas, originalWidth, originalHeight))
    {
        return response;
    }
    
    response.metadata.clear();
    response.img = image;
    response.dim = QSize(originalWidth, originalHeight);

    //Convert metadatas
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
