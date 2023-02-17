#pragma once

#include <QObject>
#include <QRunnable>
#include <QUrl>

#include <aliceVision/sfm/pipeline/regionsIO.hpp>
#include <aliceVision/sfmData/SfMData.hpp>

namespace qtAliceVision
{

/**
 * @brief QObject wrapper around a PointFeature.
 */
class MFeature : public QObject
{
    Q_OBJECT

    Q_PROPERTY(float x READ x CONSTANT)
    Q_PROPERTY(float y READ y CONSTANT)
    Q_PROPERTY(float scale READ scale CONSTANT)
    Q_PROPERTY(float orientation READ orientation CONSTANT)
    Q_PROPERTY(int landmarkId READ landmarkId CONSTANT)
    Q_PROPERTY(int trackId READ trackId CONSTANT)
    Q_PROPERTY(float rx READ rx CONSTANT)
    Q_PROPERTY(float ry READ ry CONSTANT)

public:
    MFeature() = default;
    MFeature(const MFeature& other)
        : QObject()
    {
        _feat = other._feat;
    }

    explicit MFeature(const aliceVision::feature::PointFeature& feat, QObject* parent = nullptr)
        : QObject(parent)
        , _feat(feat)
    {
    }

    void clearLandmarkInfo() { setLandmarkInfo(-1, aliceVision::Vec2f(.0f, .0f)); }

    void setLandmarkInfo(int landmarkId, const aliceVision::Vec2f& reprojection)
    {
        _landmarkId = landmarkId;
        _reprojection = reprojection;
    }

    void setReprojection(const aliceVision::Vec2f& reprojection) { _reprojection = reprojection; }

    void clearTrack() { setTrackId(-1); }
    void setTrackId(int trackId) { _trackId = trackId; }
    inline float x() const { return _feat.x(); }
    inline float y() const { return _feat.y(); }
    inline float scale() const { return _feat.scale(); }
    inline float orientation() const { return _feat.orientation(); }

    inline int landmarkId() const { return _landmarkId; }
    inline int trackId() const { return _trackId; }
    inline float rx() const { return _reprojection(0); }
    inline float ry() const { return _reprojection(1); }

    const aliceVision::feature::PointFeature& pointFeature() const { return _feat; }

private:
    int _landmarkId = -1;
    int _trackId = -1;
    aliceVision::Vec2f _reprojection{0.0f, 0.0f};
    aliceVision::feature::PointFeature _feat;
};

/**
 * @brief QRunnable object dedicated to load features using AliceVision.
 */
class FeatureIORunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:
    /// io parameters: folder, viewId, describerType
    using IOParams = std::tuple<QUrl, aliceVision::IndexT, QString>;

    explicit FeatureIORunnable(const IOParams& params)
        : _params(params)
    {
    }

    /// Load features based on input parameters
    Q_SLOT void run() override;

    /**
     * @brief  Emitted when features have been loaded and Features objects created.
     * @warning Features objects are not parented - their deletion must be handled manually.
     *
     * @param features the loaded Features list
     */
    Q_SIGNAL void resultReady(QList<MFeature*> features);

private:
    IOParams _params;
};

} // namespace qtAliceVision
