// This file is part of the AliceVision project.
// Copyright (c) 2022 AliceVision contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/pixelTypes.hpp>
#include <aliceVision/image/io.hpp>
#include <aliceVision/image/imageAlgo.hpp>

#include <aliceVision/system/Logger.hpp>
#include <aliceVision/utils/filesIO.hpp>

#include <boost/functional/hash.hpp>

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <functional>
#include <list>
#include <mutex>
#include <thread>
#include <algorithm>

namespace aliceVision {
namespace image {

/**
 * @brief A struct used to identify a cached image using its file description, color type info and downscale level.
 */
struct CacheKey
{
    std::string filename;
    int nbChannels;
    oiio::TypeDesc::BASETYPE typeDesc;
    double resizeRatio;
    std::time_t lastWriteTime;

    CacheKey(const std::string& path, int nchannels, oiio::TypeDesc::BASETYPE baseType, double ratio, std::time_t time)
      : filename(path),
        nbChannels(nchannels),
        typeDesc(baseType),
        resizeRatio(ratio),
        lastWriteTime(time)
    {

    }

    bool operator==(const CacheKey& other) const
    {
        return (filename == other.filename && 
                nbChannels == other.nbChannels && 
                typeDesc == other.typeDesc &&
                resizeRatio == other.resizeRatio && 
                lastWriteTime == other.lastWriteTime);
    }
};

struct CacheKeyHasher
{
    std::size_t operator()(const CacheKey& key) const noexcept
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, key.filename);
        boost::hash_combine(seed, key.nbChannels);
        boost::hash_combine(seed, key.typeDesc);
        boost::hash_combine(seed, key.resizeRatio);
        boost::hash_combine(seed, key.lastWriteTime);
        return seed;
    }
};

/**
 * @brief A class to support shared pointers for all types of images.
 */
class CacheValue
{
  public:
    template<typename TPix>
    CacheValue(unsigned frameId, std::shared_ptr<Image<TPix>> img) : 
    _vimg(img),
    _frameId(frameId)
    {
    }

  public:
    /**
     * @brief Template method to get a shared pointer to the image with pixel type given as template argument.
     * @note At most one of the generated methods will provide a non-null pointer.
     * @return shared pointer to an image with the pixel type given as template argument
     */
    template<typename TPix>
    std::shared_ptr<Image<TPix>> get() const
    {
        return std::get<std::shared_ptr<Image<TPix>>>(_vimg);
    }

    unsigned getOriginalWidth() const
    {
        return _originalWidth;
    }

    unsigned getOriginalHeight() const
    {
        return _originalHeight;
    }

    void setOriginalWidth(unsigned width)
    {
        _originalWidth = width;
    }

    void setOriginalHeight(unsigned height)
    {
        _originalHeight = height;
    }

    oiio::ParamValueList & getMetadatas()
    {
        return _metadatas;
    }

    unsigned getFrameId() const 
    {
        return _frameId;
    }

    /**
     * @brief Count the number of usages of the wrapped shared pointer.
     * @return the use_count of the wrapped shared pointer if there is one, otherwise 0
     */
    long int useCount() const
    {
        return std::visit([](const auto & arg) -> long int {return arg.use_count();}, _vimg);
    }

    /**
     * @brief Retrieve the memory size (in bytes) of the wrapped image.
     * @return the memory size of the wrapped image if there is one, otherwise 0
     */
    unsigned long long int memorySize() const
    {
        return std::visit([](const auto & arg) -> unsigned long long int {return arg->memorySize();}, _vimg);
    }

  private:
    std::variant<
        std::shared_ptr<Image<unsigned char>>,
        std::shared_ptr<Image<float>>,
        std::shared_ptr<Image<RGBColor>>,
        std::shared_ptr<Image<RGBAColor>>,
        std::shared_ptr<Image<RGBfColor>>,
        std::shared_ptr<Image<RGBAfColor>>
        > _vimg;

    unsigned _originalWidth;
    unsigned _originalHeight;
    oiio::ParamValueList _metadatas;
    unsigned _frameId;
};


/**
 * @brief A struct to store information about the cache current state and usage.
 */
class CacheInfo
{
public:
    CacheInfo(unsigned long int maxSize)
      : _maxSize(maxSize)
    {
    }

    void incrementCache()
    {
        const  std::scoped_lock<std::mutex> lockPeek(_mutex);
        _nbLoadFromCache++;
    }

    void incrementDisk()
    {
        const std::scoped_lock<std::mutex> lock(_mutex);
        _nbLoadFromDisk++;
    }

    unsigned long long int getCapacity() const
    {
        const std::scoped_lock<std::mutex> lock(_mutex);
        return _maxSize;
    }

    void update(const std::unordered_map<CacheKey, CacheValue, CacheKeyHasher> & images)
    {
        std::scoped_lock<std::mutex> lock(_mutex);

        _contentSize = 0;
        for (const auto & [key, value] : images)
        {
            _contentSize += value.memorySize();
            _nbImages++;
        }
    }

    size_t getAvailableSize() const
    {
        const std::scoped_lock<std::mutex> lock(_mutex);

        if (_maxSize <= _contentSize) 
        {
            return 0;
        }

        return _maxSize - _contentSize;
    }

    bool isSmallEnough(size_t value) const 
    {
        const std::scoped_lock<std::mutex> lock(_mutex);
        
        return (_contentSize + value < _maxSize);
    }

    unsigned long long int getContentSize() const
    {
        const std::scoped_lock<std::mutex> lock(_mutex);
        return _contentSize;
    }

    int getLoadFromDisk() const 
    {
        const std::scoped_lock<std::mutex> lock(_mutex);
        return _nbLoadFromDisk;
    }

    void setMaxMemory(unsigned long long int maxSize)
    {
        std::scoped_lock<std::mutex> lock(_mutex);
        _maxSize = maxSize;
    }

    /// memory usage limits
    unsigned long long int _maxSize;

    /// current state of the cache
    int _nbImages = 0;
    unsigned long long int _contentSize = 0;

    /// usage statistics
    int _nbLoadFromDisk = 0;
    int _nbLoadFromCache = 0;
    int _nbRemoveUnused = 0;

    mutable std::mutex _mutex;
};


class ImageCache
{
  public:
    using uptr = std::unique_ptr<ImageCache>;

  public:
    /**
     * @brief Create a new image cache by defining memory usage limits and image reading options.
     * @param[in] maxSize the cache maximal size (in bytes)
     * @param[in] options the reading options that will be used when loading images through this cache
     */
    ImageCache(unsigned long maxSize, const ImageReadOptions& options);

    /**
     * @brief Destroy the cache and the unused images it contains.
     */
    ~ImageCache();

    /// make image cache class non-copyable
    ImageCache(const ImageCache&) = delete;
    ImageCache& operator=(const ImageCache&) = delete;

    /**
     * @brief Retrieve a cached image at a given downscale level.
     * @note This method is thread-safe.
     * @param[in] filename the image's filename on disk
     * @param[in] frameId additional data
     * @param[in] resizeRatio the resize ratio of the image
     * @param[in] cachedOnly if true, only return images that are already in the cache
     * @return a shared pointer to the cached image
     */
    template<typename TPix>
    std::optional<CacheValue> get(const std::string& filename, unsigned frameId, double resizeRatio = 1.0, bool cachedOnly = false);

    /**
     * @brief Check if an image at a given downscale level is currently in the cache.
     * @note This method is thread-safe.
     * @param[in] filename the image's filename on disk
     * @param[in] resizeRatio  the resize ratio of the image
     * @return whether or not the cache currently contains the image
     */
    template<typename TPix>
    bool contains(const std::string& filename, double resizeRatio = 1.0) const;

    /**
     * Ask for more room, by deleting the LRU items which are not used
     * @param requestedSize the required size for the new image
     * @param toAdd the key of the image to add after cleanup
    */
    void cleanup(size_t requestedSize, const CacheKey & toAdd);


    /**
     * @return information on the current cache state and usage
     */
    inline const CacheInfo& info() const { return _info; }

    /**
     * @return the image reading options of the cache
     */
    inline const ImageReadOptions& readOptions() const { return _options; }

    /**
     * @brief update the cache max memory
     * @param maxSize the value to store
    */
    void updateMaxMemory(unsigned long long int maxSize);

    /**
     * @brief set the reference frame ID
     * @param referenceFrameId the value to store
    */
    void setReferenceFrameId(int referenceFrameId);

  private:
    /**
     * @brief Load a new image corresponding to the given key and add it as a new entry in the cache.
     * @param[in] key the key used to identify the entry in the cache
     * @param[in] frameId additional data
     */
    template<typename TPix>
    std::optional<CacheValue> load(const CacheKey& key, unsigned frameId);

    CacheInfo _info;
    ImageReadOptions _options;

    //Set of images stored and indexed by CacheKey
    std::unordered_map<CacheKey, CacheValue, CacheKeyHasher> _imagePtrs;
    mutable std::mutex _mutexAccessImages;

    //Reference frame Id used to compute the next image to remove
    //This should be equal to the currently displayed image
    std::atomic<int> _referenceFrameId;
};

// Since some methods in the ImageCache class are templated
// their definition must be given in this header file

template<typename TPix>
std::optional<CacheValue> ImageCache::get(const std::string& filename, unsigned frameId, double resizeRatio, bool cachedOnly)
{
    if (resizeRatio < 1e-12)
    {
        return std::nullopt;
    }

    //Build lookup key
    using TInfo = ColorTypeInfo<TPix>;
    auto lastWriteTime = utils::getLastWriteTime(filename);
    CacheKey keyReq(filename, TInfo::size, TInfo::typeDesc, resizeRatio, lastWriteTime);

    // find the requested image in the cached images
    {
        std::scoped_lock<std::mutex> lockImages(_mutexAccessImages);
        auto it = _imagePtrs.find(keyReq);
        if (it != _imagePtrs.end())
        {
            return it->second;
        }
    }

    if (cachedOnly)
    {
        return std::nullopt;
    }
    
    //Load image and add to cache if possible
    return load<TPix>(keyReq, frameId);
}

template<typename TPix>
std::optional<CacheValue> ImageCache::load(const CacheKey& key, unsigned frameId)
{
    Image<TPix> img;
    auto resized = std::make_shared<Image<TPix>>();

    int width = 0;
    int height = 0;
    oiio::ParamValueList metadatas;

    try 
    {
        metadatas = readImageMetadata(key.filename, width, height);
        
        // load image from disk
        readImage(key.filename, img, _options);
    }
    catch (...)
    {
        return std::nullopt;
    }


    // Compute new size, make sure the size is at least 1
    double dw = key.resizeRatio * double(img.width());
    double dh = key.resizeRatio * double(img.height());
    int tw = static_cast<int>(std::max(1, int(std::ceil(dw))));
    int th = static_cast<int>(std::max(1, int(std::ceil(dh))));

    using TInfo = ColorTypeInfo<TPix>;
    cleanup(tw*th*size_t(TInfo::size), key);
    
    // apply downscale
    imageAlgo::resizeImage(tw, th, img, *resized);

    //Increment disk access stats
    _info.incrementDisk();

    // create wrapper around shared pointer
    CacheValue value(frameId, resized);

    //Add additional information about the image
    value.setOriginalHeight(static_cast<unsigned int>(height));
    value.setOriginalWidth(static_cast<unsigned int>(width));
    value.getMetadatas() = metadatas;

    // Store image in map
    {
        std::scoped_lock<std::mutex> lockImages(_mutexAccessImages);
        
        _imagePtrs.insert({key, value});
        _info.update(_imagePtrs);
    }

    return value; 
}

template<typename TPix>
bool ImageCache::contains(const std::string& filename, double resizeRatio) const
{
    std::scoped_lock<std::mutex> lockKeys(_mutexAccessImages);

    using TInfo = ColorTypeInfo<TPix>;
    auto lastWriteTime = utils::getLastWriteTime(filename);
    CacheKey keyReq(filename, TInfo::size, TInfo::typeDesc, resizeRatio, lastWriteTime);
    auto it = _imagePtrs.find(keyReq);

    bool found = (it != _imagePtrs.end());

    return found;
}



}  // namespace image
}  // namespace aliceVision
