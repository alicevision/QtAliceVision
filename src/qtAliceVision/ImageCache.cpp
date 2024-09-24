// This file is part of the AliceVision project.
// Copyright (c) 2022 AliceVision contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ImageCache.hpp"

#include <aliceVision/system/Logger.hpp>

namespace aliceVision {
namespace image {

ImageCache::ImageCache(unsigned long maxSize, const ImageReadOptions& options)
  : _info(maxSize),
    _options(options),
    _referenceFrameId(0)
{}

ImageCache::~ImageCache() {}

void ImageCache::cleanup(size_t requestedSize, const CacheKey & toAdd)
{
    //At each step, we try to remove the LRU item which is not used
    while (1)
    {
        //Check if we did enough work ?
        size_t available = _info.getAvailableSize();
        if (available >= requestedSize)
        {
            return;
        }


        bool erased = false;

        /* First, try to remove images with different ratios **/
        {
            std::scoped_lock<std::mutex> lockKeys(_mutexAccessImages);
            for (const auto & [key, value] : _imagePtrs)
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

        //if we arrive here, all the cache should contains only the same resize ratio
        if (!erased)
        {
            std::scoped_lock<std::mutex> lockKeys(_mutexAccessImages);

            std::map<int, const CacheKey*> orderedKeys;

            for (const auto & [key, value] : _imagePtrs)
            {
                int iOtherId = int(value.getFrameId());
                int diff = iOtherId - _referenceFrameId;

                //Before the frameId, difference is negative.
                //The closest it is to the frameId before the frameid, the highest its priority to delete
                //After the frameId, the largest the difference, the highest its priority to delete
                if (diff < 0)
                {
                    diff = std::numeric_limits<int>::max() + diff;
                }

                orderedKeys[diff] = &key;
            }

            if (orderedKeys.size() > 0)
            {
                const CacheKey * pKey = orderedKeys.rbegin()->second;
                _imagePtrs.erase(*pKey);
                _info.update(_imagePtrs);
            }
        }

        //Nothing happened, nothing more will happen.
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

void ImageCache::setReferenceFrameId(int referenceFrameId)
{
    _referenceFrameId = referenceFrameId;
}

}  // namespace image
}  // namespace aliceVision
