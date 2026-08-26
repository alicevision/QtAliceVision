#include "ImageCache.hpp"

namespace qtAliceVision {

ImageCache::ImageCache(unsigned long maxSize, const aliceVision::image::ImageReadOptions& options)
  : _info(maxSize),
    _options(options),
    _referenceFrameId(0)
{
    //Use 4 threads max, but less if we don't have this
    int count = std::min(4, omp_get_max_threads());

    //Setup openimageio
    oiio::attribute("threads", count);
    oiio::attribute("exr_threads", count);
}

ImageCache::~ImageCache() {}

void ImageCache::cleanup(size_t requestedSize, const CacheKey& toAdd)
{
    // At each step, we try to remove the LRU item which is not used
    while (1)
    {
        // Check if we did enough work?
        size_t available = _info.getAvailableSize();
        if (available >= requestedSize)
        {
            return;
        }

        bool erased = false;

        // First, try to remove images with different ratios
        {
            std::scoped_lock<std::mutex> lockKeys(_mutexAccessImages);
            for (const auto& [key, value] : _imagePtrs)
            {
                if (key.resizeRatio == toAdd.resizeRatio)
                {
                    continue;
                }

                if (value.useCount() <= 1)
                {
                    _imagePtrs.erase(key);
                    _info.update(_imagePtrs);
                    erased = true;
                    break;
                }
            }
        }

        // If we get here, all the cache should contain only the same resize ratio
        if (!erased)
        {
            std::scoped_lock<std::mutex> lockKeys(_mutexAccessImages);

            std::map<int, const CacheKey*> orderedKeys;

            for (const auto& [key, value] : _imagePtrs)
            {
                int iOtherId = int(value.getFrameId());
                int diff = iOtherId - _referenceFrameId;

                // Before the frameId, difference is negative.
                // The closest it is to the frameId before the frameId, the highest its priority to delete
                // After the frameId, the largest the difference, the highest its priority to delete
                if (diff < 0)
                {
                    diff = std::numeric_limits<int>::max() + diff;
                }

                orderedKeys[diff] = &key;
            }

            if (orderedKeys.size() > 0)
            {
                const CacheKey* pKey = orderedKeys.rbegin()->second;
                _imagePtrs.erase(*pKey);
                _info.update(_imagePtrs);
                erased = true;
            }
        }

        // Nothing happened, nothing more will happen.
        if (!erased)
        {
            return;
        }
    }
}

void ImageCache::updateMaxMemory(unsigned long long int maxSize) 
{ 
    _info.setMaxMemory(maxSize); 
}

unsigned long long int ImageCache::getMaxMemory()
{
    return _info.getCapacity();
}

void ImageCache::setReferenceFrameId(int referenceFrameId) 
{ 
    _referenceFrameId = referenceFrameId; 
}

}  // namespace qtAliceVision
