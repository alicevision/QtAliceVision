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
namespace imageio {

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
 * @brief Image caching system for loading image sequences from disk.
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

    explicit SequenceCache(QObject* parent = nullptr);

    ~SequenceCache();

    void setSequence(const QVariantList& paths);

    QVariantList getCachedFrames() const;

public:

    // Request management

    Response request(const std::string& path) override;

    Q_SLOT void onPrefetchingProgressed(int reqFrame);

    Q_SLOT void onPrefetchingDone(int reqFrame);

    Q_SIGNAL void requestHandled();

private:

    // Member variables

    std::vector<FrameData> _sequence;

    aliceVision::image::ImageCache* _cache;

    std::pair<int, int> _regionSafe;

    bool _loading;

private:

    // Utility methods

    int getFrame(const std::string& path) const;

    std::pair<int, int> getRegion(int frame, int extent) const;

};

/**
 * @brief Utility class for loading images from disk to cache asynchronously.
 */
class PrefetchingIORunnable : public QObject, public QRunnable {

    Q_OBJECT

public:

    PrefetchingIORunnable(aliceVision::image::ImageCache* cache,
                          const std::vector<FrameData>& toLoad,
                          int reqFrame,
                          double fillRatio);

    ~PrefetchingIORunnable();

    Q_SLOT void run() override;

    Q_SIGNAL void progressed(int reqFrame);

    Q_SIGNAL void done(int reqFrame);

private:

    aliceVision::image::ImageCache* _cache;

    std::vector<FrameData> _toLoad;

    int _reqFrame;

    uint64_t _toFill;

};

} // namespace imageio
} // namespace qtAliceVision
