#pragma once

#include "ImageServer.hpp"

#include <aliceVision/image/all.hpp>

#include <QObject>
#include <QRunnable>
#include <QSize>
#include <QVariant>
#include <QMap>
#include <QList>

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <cstdint>


namespace qtAliceVision {
namespace imgserve {

/**
 * @brief Utility struct for manipulating various information about a given frame.
 */
struct FrameData {

    std::string path;

    QSize dim;

    QVariantMap metadata;

    int frame;

};

/**
 * @brief Image server with a caching system for loading image sequences from disk.
 * 
 * Given a sequence of images (ordered by filename), the SequenceCache works as an image server:
 * it receives requests from clients (in the form of a filepath)
 * and its purpose is to provide the corresponding images (if they exist in the sequence).
 * 
 * The SequenceCache takes advantage of the ordering of the sequence to load whole "regions" at once
 * (a region being a contiguous range of images from the sequence).
 * Such strategy makes sense under the assumption that the sequence order is meaningful for clients,
 * i.e. that if an image is queried then it is likely that the next queries will be close in the sequence.
 */
class SequenceCache : public QObject, public ImageServer {

    Q_OBJECT

public:

    // Constructors and destructor

    explicit SequenceCache(QObject* parent = nullptr);

    ~SequenceCache();

public:

    // Data manipulation for the Qt property system

    /**
     * @brief Set the current image sequence.
     * @param[in] paths unordered list of image filepaths
     * @note the sequence order will not be changed
     */
    void setSequence(const QVariantList& paths);

    /**
     * @brief Get the frames in the sequence that are currently cached.
     * @return a list of intervals, each one describing a range of cached frames
     * @note we use QPoints to represent the intervals (x: range start, y: range end)
     */
    QVariantList getCachedFrames() const;

public:

    // Request management

    /// If the image requested falls outside a certain region of cached images,
    /// this method will launch a worker thread to prefetch new images from disk.
    ResponseData request(const RequestData& reqData) override;

    /**
     * @brief Slot called every time the prefetching thread progressed.
     * @param[in] reqFrame the frame initially requested when the worker thread was started
     */
    Q_SLOT void onPrefetchingProgressed(int reqFrame);

    /**
     * @brief Slot called when the prefetching thread is finished.
     * @param[in] reqFrame the frame initially requested when the worker thread was started
     */
    Q_SLOT void onPrefetchingDone(int reqFrame);

    /**
     * @brief Signal emitted when the prefetching thread is done and a previous request has been handled.
     */
    Q_SIGNAL void requestHandled();

private:

    // Member variables

    /// Ordered sequence of frames.
    std::vector<FrameData> _sequence;

    /// Image cache.
    aliceVision::image::ImageCache* _cache;

    /// Frame interval used to decide if a prefetching thread should be launched.
    std::pair<int, int> _regionSafe;

    /// Keep track of whether or not there is an active worker thread.
    bool _loading;

private:

    // Utility methods

    /**
     * @brief Retrieve frame number corresponding to an image in the sequence.
     * @param[in] path filepath of an image in the sequence
     * @return frame number of the queried image if it is in the sequence, otherwise -1
     */
    int getFrame(const std::string& path) const;

    /**
     * @brief Build a frame interval in the sequence.
     * @param[in] frame central frame of the interval
     * @param[in] extent interval half-size
     * @return an interval of size 2*extent that fits in the sequence and contains the given frame
     */
    std::pair<int, int> buildRegion(int frame, int extent) const;

};

/**
 * @brief Utility class for loading images from disk to cache asynchronously.
 */
class PrefetchingIORunnable : public QObject, public QRunnable {

    Q_OBJECT

public:

    /**
     * @param[in] cache pointer to image cache to fill
     * @param[in] toLoad sequence frames to load from disk
     * @param[in] reqFrame initially requested frame
     * @param[in] fillRatio proportion of cache capacity that can be filled
     */
    PrefetchingIORunnable(aliceVision::image::ImageCache* cache,
                          const std::vector<FrameData>& toLoad,
                          int reqFrame,
                          double fillRatio);

    ~PrefetchingIORunnable();

    /// Main method for loading images from disk to cache in a worker thread.
    Q_SLOT void run() override;

    /**
     * @brief Signal emitted regularly during prefetching when progress is made.
     * @param[in] reqFrame initially requested frame
     */
    Q_SIGNAL void progressed(int reqFrame);

    /**
     * @brief Signal emitted when prefetching is finished.
     * @param[in] reqFrame initially requested frame
     */
    Q_SIGNAL void done(int reqFrame);

private:

    /// Image cache to fill.
    aliceVision::image::ImageCache* _cache;

    /// Frames to load in cache.
    std::vector<FrameData> _toLoad;

    /// Initially requested frame, used as central point for loading order.
    int _reqFrame;

    /// Maximum memory that can be filled.
    uint64_t _toFill;

};

} // namespace imgserve
} // namespace qtAliceVision
