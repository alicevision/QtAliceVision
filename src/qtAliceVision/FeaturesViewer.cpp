#include "FeaturesViewer.hpp"

#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>
#include <QTransform>
#include <QtMath>

#include <vector>
#include <algorithm>
#include <limits>

namespace qtAliceVision {

FeaturesViewer::FeaturesViewer(QQuickItem* parent)
  : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);

    // trigger display repaint events
    connect(this, &FeaturesViewer::displayFeaturesChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::displayTracksChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::displayMatchesChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::displayLandmarksChanged, this, &FeaturesViewer::update);

    connect(this, &FeaturesViewer::featureDisplayModeChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::trackDisplayModeChanged, this, &FeaturesViewer::update);

    connect(this, &FeaturesViewer::featureMinScaleFilterChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::featureMaxScaleFilterChanged, this, &FeaturesViewer::update);

    connect(this, &FeaturesViewer::display3dTracksChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::trackContiguousFilterChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::trackInliersFilterChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::displayTrackEndpointsChanged, this, &FeaturesViewer::update);

    connect(this, &FeaturesViewer::featureColorChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::matchColorChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::landmarkColorChanged, this, &FeaturesViewer::update);

    connect(this, &FeaturesViewer::currentViewIdChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::enableTimeWindowChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::timeWindowChanged, this, &FeaturesViewer::update);

    // trigger data repaint events
    connect(this, &FeaturesViewer::describerTypeChanged, this, &FeaturesViewer::updateReconstruction);
    connect(this, &FeaturesViewer::featuresChanged, this, &FeaturesViewer::updateReconstruction);
    connect(this, &FeaturesViewer::tracksChanged, this, &FeaturesViewer::updateReconstruction);
    connect(this, &FeaturesViewer::sfmDataChanged, this, &FeaturesViewer::updateReconstruction);
    connect(this, &FeaturesViewer::reconstructionChanged, this, &FeaturesViewer::update);
}

FeaturesViewer::~FeaturesViewer() {}

void FeaturesViewer::setMFeatures(MFeatures* features)
{
    if (_mfeatures == features)
        return;

    if (_mfeatures != nullptr)
    {
        disconnect(_mfeatures, SIGNAL(featuresChanged()), this, SIGNAL(featuresChanged()));
    }

    _mfeatures = features;

    if (_mfeatures != nullptr)
    {
        connect(_mfeatures, SIGNAL(featuresChanged()), this, SIGNAL(featuresChanged()));
    }
    Q_EMIT featuresChanged();
}

void FeaturesViewer::setMTracks(MTracks* tracks)
{
    if (_mtracks == tracks)
        return;

    if (_mtracks != nullptr)
    {
        disconnect(_mtracks, SIGNAL(tracksChanged()), this, SIGNAL(tracksChanged()));
    }

    _mtracks = tracks;

    if (_mtracks != nullptr)
    {
        connect(_mtracks, SIGNAL(tracksChanged()), this, SIGNAL(tracksChanged()));
    }
    Q_EMIT tracksChanged();
}

void FeaturesViewer::setMSfMData(MSfMData* sfmData)
{
    if (_msfmdata == sfmData)
        return;

    if (_msfmdata != nullptr)
    {
        disconnect(_msfmdata, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }

    _msfmdata = sfmData;

    if (_msfmdata != nullptr)
    {
        connect(_msfmdata, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    Q_EMIT sfmDataChanged();
}

void FeaturesViewer::updatePaintFeatures(const PaintParams& params, QSGNode* node)
{
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " features.";

    if (!_displayFeatures || !params.haveValidFeatures)
    {
        // nothing to draw or something is not ready
        painter.clearLayer(node, "features");
        return;
    }

    switch (_featureDisplayMode)
    {
        case FeaturesViewer::Points:
            paintFeaturesAsPoints(params, node);
            break;
        case FeaturesViewer::Squares:
            paintFeaturesAsSquares(params, node);
            break;
        case FeaturesViewer::OrientedSquares:
            paintFeaturesAsOrientedSquares(params, node);
            break;
    }
}

void FeaturesViewer::paintFeaturesAsPoints(const PaintParams& params, QSGNode* node)
{
    std::vector<QPointF> points;

    const auto currentViewFeaturesIt = _mreconstruction.featureDatasPerView.find(_currentViewId);
    if (currentViewFeaturesIt == _mreconstruction.featureDatasPerView.end())
    {
        return;
    }

    const auto& currentViewFeatures = currentViewFeaturesIt->second;

    for (const auto& feature : currentViewFeatures)
    {
        // feature scale filter
        if (feature.scale > params.maxFeatureScale || feature.scale < params.minFeatureScale)
        {
            continue;
        }

        points.emplace_back(feature.x, feature.y);
    }

    painter.drawPoints(node, "features", points, _featureColor, 6.f);
}

void FeaturesViewer::paintFeaturesAsSquares(const PaintParams& params, QSGNode* node)
{
    std::vector<QPointF> points;

    const auto currentViewFeaturesIt = _mreconstruction.featureDatasPerView.find(_currentViewId);
    if (currentViewFeaturesIt == _mreconstruction.featureDatasPerView.end())
    {
        return;
    }

    const auto& currentViewFeatures = currentViewFeaturesIt->second;

    for (const auto& feature : currentViewFeatures)
    {
        // feature scale filter
        if (feature.scale > params.maxFeatureScale || feature.scale < params.minFeatureScale)
        {
            continue;
        }

        const float radius = feature.scale;
        const double diag = 2.0 * feature.scale;

        QRectF rect(feature.x, feature.y, diag, diag);
        rect.translate(-radius, -radius);
        QPointF tl = rect.topLeft();
        QPointF tr = rect.topRight();
        QPointF br = rect.bottomRight();
        QPointF bl = rect.bottomLeft();

        points.push_back(tl);
        points.push_back(tr);
        points.push_back(br);
        points.push_back(br);
        points.push_back(bl);
        points.push_back(tl);
    }

    painter.drawTriangles(node, "features", points, _featureColor);
}

void FeaturesViewer::paintFeaturesAsOrientedSquares(const PaintParams& params, QSGNode* node)
{
    std::vector<QPointF> points;

    const auto currentViewFeaturesIt = _mreconstruction.featureDatasPerView.find(_currentViewId);
    if (currentViewFeaturesIt == _mreconstruction.featureDatasPerView.end())
    {
        return;
    }

    const auto& currentViewFeatures = currentViewFeaturesIt->second;

    for (const auto& feature : currentViewFeatures)
    {
        // feature scale filter
        if (feature.scale > params.maxFeatureScale || feature.scale < params.minFeatureScale)
        {
            continue;
        }

        const float radius = feature.scale;
        const double diag = 2.0 * feature.scale;

        QRectF rect(feature.x, feature.y, diag, diag);
        rect.translate(-radius, -radius);
        QPointF tl = rect.topLeft();
        QPointF tr = rect.topRight();
        QPointF br = rect.bottomRight();
        QPointF bl = rect.bottomLeft();

        // compute angle: use feature angle and remove self rotation
        const auto radAngle = -feature.orientation - qDegreesToRadians(rotation());
        // generate a QTransform that represent this rotation
        const auto t = QTransform().translate(feature.x, feature.y).rotateRadians(radAngle).translate(-feature.x, -feature.y);

        // create lines, each vertice has to be duplicated (A->B, B->C, C->D, D->A) since we use GL_LINES
        std::vector<QPointF> corners = {t.map(tl), t.map(tr), t.map(br), t.map(bl)};
        for (unsigned int k = 0; k < corners.size(); ++k)
        {
            points.emplace_back(corners[k]);
            points.emplace_back(corners[(k + 1) % corners.size()]);
        }
        // orientation line: up vector (0, 1)
        auto o2 = t.map(rect.center() - QPointF(0.0f, radius));  // rotate end point
        points.emplace_back(rect.center());
        points.emplace_back(o2);
    }

    painter.drawLines(node, "features", points, _featureColor, 2.f);
}

void FeaturesViewer::updatePaintTracks(const PaintParams& params, QSGNode* node)
{
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " tracks.";

    bool valid = (_displayTracks && params.haveValidFeatures && params.haveValidTracks && params.haveValidLandmarks && _msfmdata &&
                  _currentViewId != aliceVision::UndefinedIndexT);

    aliceVision::IndexT currentFrameId = aliceVision::UndefinedIndexT;
    if (valid)
    {
        if (_msfmdata->rawData().getViews().count(_currentViewId))
        {
            const auto& view = _msfmdata->rawData().getView(_currentViewId);
            currentFrameId = view.getFrameId();
        }

        if (currentFrameId == aliceVision::UndefinedIndexT)
        {
            qInfo() << "[QtAliceVision] FeaturesViewer: Unable to update paint " << _describerType << " tracks, can't find current frame id.";
            valid = false;
        }
    }

    if (!valid)
    {
        painter.clearLayer(node, "trackEndpoints");
        painter.clearLayer(node, "highlightPoints");
        painter.clearLayer(node, "trackLines_gaps");
        painter.clearLayer(node, "trackLines_reconstruction_none");
        painter.clearLayer(node, "trackLines_reconstruction_partial_outliers");
        painter.clearLayer(node, "trackLines_reconstruction_partial_inliers");
        painter.clearLayer(node, "trackLines_reconstruction_full");
        painter.clearLayer(node, "trackPoints_outliers");
        painter.clearLayer(node, "trackPoints_inliers");
        return;
    }

    std::vector<QPointF> trackEndpoints;
    std::vector<QPointF> highlightPoints;
    std::vector<QPointF> trackLines_reconstruction_none;
    std::vector<QPointF> trackLines_reconstruction_partial_outliers;
    std::vector<QPointF> trackLines_reconstruction_partial_inliers;
    std::vector<QPointF> trackLines_reconstruction_full;
    std::vector<QPointF> trackLines_gaps;
    std::vector<QPointF> trackPoints_outliers;
    std::vector<QPointF> trackPoints_inliers;

    for (const auto& track : _mreconstruction.trackDatas)
    {
        // check that track has at least 2 features
        if (track.elements.size() < 2)
        {
            continue;
        }

        // apply scale filter
        if (track.averageScale < params.minFeatureScale || track.averageScale > params.maxFeatureScale)
        {
            continue;
        }

        // retrieve track reconstruction state
        const bool trackHasInliers = (track.nbReconstructed > 0);
        const bool trackFullyReconstructed = (static_cast<std::size_t>(track.nbReconstructed) == track.elements.size());

        // apply inlier filter
        if (!trackHasInliers && _trackInliersFilter)
        {
            continue;
        }

        // check that track frame interval contains current frame
        const aliceVision::IndexT startFrameId = track.elements.front().frameId;
        const aliceVision::IndexT endFrameId = track.elements.back().frameId;
        if (currentFrameId < startFrameId || currentFrameId > endFrameId)
        {
            continue;
        }

        // utility variables to store data between iterations
        MReconstruction::FeatureData previousFeature;
        aliceVision::IndexT previousFrameId = aliceVision::UndefinedIndexT;
        bool previousFeatureInlier = false;

        for (const auto& elt : track.elements)
        {
            // check that frameId is in timeWindow if enabled
            if (_enableTimeWindow && (_timeWindow >= 0) &&
                (elt.frameId < currentFrameId - static_cast<aliceVision::IndexT>(_timeWindow) ||
                 elt.frameId > currentFrameId + static_cast<aliceVision::IndexT>(_timeWindow)))
            {
                continue;
            }

            // retrieve feature data
            const auto& viewFeatures = _mreconstruction.featureDatasPerView.at(elt.viewId);
            const auto& currentFeature = viewFeatures[elt.featureId];

            // retrieve whether or not current feature has a corresponding observation
            const bool currentFeatureInlier = currentFeature.hasLandmark;

            if (previousFrameId != aliceVision::UndefinedIndexT)
            {
                // The 2 features of the track are contiguous
                const bool contiguous = (previousFrameId == (elt.frameId - 1));
                // The 2 features of the track are resectioning inliers
                const bool inliers = previousFeatureInlier && currentFeatureInlier;

                // geometry
                const QPointF prevPoint = (_display3dTracks && previousFeatureInlier) ? QPointF(previousFeature.rx, previousFeature.ry)
                                                                                      : QPointF(previousFeature.x, previousFeature.y);
                const QPointF curPoint = (_display3dTracks && currentFeatureInlier) ? QPointF(currentFeature.rx, currentFeature.ry)
                                                                                    : QPointF(currentFeature.x, currentFeature.y);

                // track line
                if (!contiguous)
                {
                    trackLines_gaps.push_back(prevPoint);
                    trackLines_gaps.push_back(curPoint);
                }
                else if (!trackHasInliers && !_trackInliersFilter)
                {
                    trackLines_reconstruction_none.push_back(prevPoint);
                    trackLines_reconstruction_none.push_back(curPoint);
                }
                else if (!trackFullyReconstructed)
                {
                    if (inliers)
                    {
                        trackLines_reconstruction_partial_inliers.push_back(prevPoint);
                        trackLines_reconstruction_partial_inliers.push_back(curPoint);
                    }
                    else
                    {
                        trackLines_reconstruction_partial_outliers.push_back(prevPoint);
                        trackLines_reconstruction_partial_outliers.push_back(curPoint);
                    }
                }
                else if (trackFullyReconstructed)
                {
                    trackLines_reconstruction_full.push_back(prevPoint);
                    trackLines_reconstruction_full.push_back(curPoint);
                }

                // highlight point
                if (_trackDisplayMode == WithAllMatches || _trackDisplayMode == WithCurrentMatches)
                {
                    if (previousFrameId == currentFrameId)
                    {
                        highlightPoints.push_back(prevPoint);
                    }
                    else if (elt.frameId == currentFrameId)
                    {
                        highlightPoints.push_back(curPoint);
                    }
                }

                // track points
                // current point
                if (_trackDisplayMode == WithAllMatches || (elt.frameId == currentFrameId && _trackDisplayMode == WithCurrentMatches))
                {
                    if (currentFeatureInlier)
                    {
                        trackPoints_inliers.push_back(curPoint);
                    }
                    else
                    {
                        trackPoints_outliers.push_back(curPoint);
                    }
                }
                // first point in time window
                if ((_trackDisplayMode == WithAllMatches && previousFrameId == startFrameId) ||
                    (_trackDisplayMode == WithCurrentMatches && previousFrameId == currentFrameId))
                {
                    if (previousFeatureInlier)
                    {
                        trackPoints_inliers.push_back(prevPoint);
                    }
                    else
                    {
                        trackPoints_outliers.push_back(prevPoint);
                    }
                }

                if (_displayTrackEndpoints)
                {
                    // draw global start point
                    if (previousFrameId == startFrameId)
                    {
                        QPointF triangle[] = {QPointF(0, 0), QPointF(-2, 1), QPointF(-2, -1)};
                        double angle = QLineF(prevPoint, curPoint).angle() - rotation();
                        auto tr = QTransform().rotate(-angle).scale(10, 10);
                        for (int i = 0; i < 3; ++i)
                            trackEndpoints.push_back(prevPoint + tr.map(triangle[i]));
                    }

                    // draw global end point
                    if (elt.frameId == endFrameId)
                    {
                        QPointF triangle[] = {QPointF(0, 0), QPointF(-2, 1), QPointF(-2, -1)};
                        double angle = QLineF(curPoint, prevPoint).angle() - rotation();
                        auto tr = QTransform().rotate(-angle).scale(10, 10);
                        for (int i = 0; i < 3; ++i)
                            trackEndpoints.push_back(curPoint + tr.map(triangle[i]));
                    }
                }
            }

            // frame id is now previous frame id
            previousFrameId = elt.frameId;
            previousFeature = currentFeature;
            previousFeatureInlier = currentFeatureInlier;
        }
    }

    painter.drawTriangles(node, "trackEndpoints", trackEndpoints, _endpointColor);
    painter.drawPoints(node, "highlightPoints", highlightPoints, QColor(255, 255, 255), 6.f);
    painter.drawLines(node, "trackLines_reconstruction_none", trackLines_reconstruction_none, _featureColor, 2.f);
    painter.drawLines(node, "trackLines_reconstruction_partial_outliers", trackLines_reconstruction_partial_outliers, _matchColor, 2.f);
    painter.drawLines(node, "trackLines_reconstruction_partial_inliers", trackLines_reconstruction_partial_inliers, _landmarkColor, 2.f);
    painter.drawLines(node, "trackLines_reconstruction_full", trackLines_reconstruction_full, _landmarkColor, 5.f);
    painter.drawLines(node, "trackLines_gaps", trackLines_gaps, _trackContiguousFilter ? QColor(0, 0, 0, 0) : QColor(50, 50, 50), 2.f);
    painter.drawPoints(node, "trackPoints_outliers", trackPoints_outliers, _matchColor, 4.f);
    painter.drawPoints(node, "trackPoints_inliers", trackPoints_inliers, _landmarkColor, 4.f);
}

void FeaturesViewer::updatePaintMatches(const PaintParams& params, QSGNode* node)
{
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " matches.";

    if (!_displayMatches || !params.haveValidFeatures || !params.haveValidTracks)
    {
        // nothing to draw or something is not ready
        painter.clearLayer(node, "matches");
        return;
    }

    std::vector<QPointF> points;

    const auto currentViewFeaturesIt = _mreconstruction.featureDatasPerView.find(_currentViewId);
    if (currentViewFeaturesIt == _mreconstruction.featureDatasPerView.end())
    {
        return;
    }

    const auto& currentViewFeatures = currentViewFeaturesIt->second;

    for (const auto& feature : currentViewFeatures)
    {
        if (feature.scale > params.maxFeatureScale || feature.scale < params.minFeatureScale)
        {
            continue;
        }

        if (!feature.hasTrack)
        {
            continue;
        }

        if (feature.hasLandmark && _displayLandmarks)
        {
            continue;
        }

        points.emplace_back(feature.x, feature.y);
    }

    painter.drawPoints(node, "matches", points, _matchColor, 6.f);
}

void FeaturesViewer::updatePaintLandmarks(const PaintParams& params, QSGNode* node)
{
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " landmarks.";

    if (!_displayLandmarks || !params.haveValidFeatures || !params.haveValidLandmarks)
    {
        // nothing to draw or something is not ready
        painter.clearLayer(node, "reprojectionErrors");
        painter.clearLayer(node, "landmarks");
        return;
    }

    std::vector<QPointF> points;
    std::vector<QPointF> lines;

    const auto currentViewFeaturesIt = _mreconstruction.featureDatasPerView.find(_currentViewId);
    if (currentViewFeaturesIt == _mreconstruction.featureDatasPerView.end())
    {
        return;
    }

    const auto& currentViewFeatures = currentViewFeaturesIt->second;

    for (const auto& feature : currentViewFeatures)
    {
        if (feature.scale > params.maxFeatureScale || feature.scale < params.minFeatureScale)
        {
            continue;
        }

        if (!feature.hasLandmark)
        {
            continue;
        }

        lines.emplace_back(feature.x, feature.y);
        lines.emplace_back(feature.rx, feature.ry);
        points.emplace_back(feature.rx, feature.ry);
    }

    const QColor reprojectionColor = _landmarkColor.darker(150);
    painter.drawLines(node, "reprojectionErrors", lines, reprojectionColor, 1.f);

    painter.drawPoints(node, "landmarks", points, _landmarkColor, 6.f);
}

void FeaturesViewer::initializePaintParams(PaintParams& params)
{
    params.haveValidFeatures = _mfeatures ? _mfeatures->rawDataPtr() != nullptr && _mfeatures->status() == MFeatures::Ready : false;

    if (!params.haveValidFeatures)
        return;

    params.haveValidTracks = _mtracks ? _mtracks->tracksPtr() != nullptr && _mtracks->status() == MTracks::Ready : false;

    params.haveValidLandmarks = _msfmdata ? _msfmdata->rawDataPtr() != nullptr && _msfmdata->status() == MSfMData::Ready : false;

    const float minFeatureScale = _mreconstruction.minFeatureScale;
    const float difFeatureScale = _mreconstruction.maxFeatureScale - minFeatureScale;

    params.minFeatureScale = minFeatureScale + std::max(0.f, std::min(1.f, _featureMinScaleFilter)) * difFeatureScale;
    params.maxFeatureScale = minFeatureScale + std::max(0.f, std::min(1.f, _featureMaxScaleFilter)) * difFeatureScale;
}

QSGNode* FeaturesViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
{
    (void)data;  // Fix "unused parameter" warnings; should be replaced by [[maybe_unused]] when C++17 is supported
    PaintParams params;
    initializePaintParams(params);

    // Implementation remarks
    // Only one QSGGeometryNode containing all geometry needed to draw all features
    // is created. This is ATM the only solution that scales well and provides
    // good performance even for +100K feature points.
    // The number of created vertices varies depending on the selected display mode.

    QSGNode* node = oldNode;
    if (!node)
        node = new QSGNode;

    updatePaintFeatures(params, node);
    updatePaintTracks(params, node);
    updatePaintMatches(params, node);
    updatePaintLandmarks(params, node);

    return node;
}

void FeaturesViewer::updateReconstruction()
{
    // clear previous data

    _mreconstruction.featureDatasPerView.clear();
    _mreconstruction.trackDatas.clear();
    _mreconstruction.minFeatureScale = 0.f;
    _mreconstruction.maxFeatureScale = std::numeric_limits<float>::max();

    // check validity of the different data sources

    bool validFeatures = _mfeatures ? _mfeatures->rawDataPtr() != nullptr && _mfeatures->status() == MFeatures::Ready : false;

    bool validTracks = _mtracks ? _mtracks->tracksPtr() != nullptr && _mtracks->status() == MTracks::Ready : false;

    bool validLandmarks = _msfmdata ? _msfmdata->rawDataPtr() != nullptr && _msfmdata->status() == MSfMData::Ready : false;

    // features

    if (validFeatures)
    {
        const auto& featuresPerViewPerDesc = _mfeatures->rawData();
        const auto featuresPerViewIt = featuresPerViewPerDesc.find(_describerType.toStdString());
        if (featuresPerViewIt == featuresPerViewPerDesc.end())
        {
            return;
        }
        const auto& featuresPerView = featuresPerViewIt->second;

        float minScale = std::numeric_limits<float>::max();
        float maxScale = 0.f;

        for (const auto& [viewId, features] : featuresPerView)
        {
            std::vector<MReconstruction::FeatureData> featureDatas;

            for (const auto& feature : features)
            {
                MReconstruction::FeatureData data;
                data.x = feature.x();
                data.y = feature.y();
                data.rx = data.x;
                data.ry = data.y;
                data.scale = feature.scale();
                data.orientation = feature.orientation();
                data.hasTrack = false;
                data.hasLandmark = false;
                featureDatas.push_back(data);

                minScale = std::min(minScale, data.scale);
                maxScale = std::max(maxScale, data.scale);
            }

            _mreconstruction.featureDatasPerView[viewId] = featureDatas;
        }

        _mreconstruction.minFeatureScale = minScale;
        _mreconstruction.maxFeatureScale = maxScale;
    }
    else
    {
        return;
    }

    // sfmdata

    if (validLandmarks)
    {
        const auto& sfmData = _msfmdata->rawData();
        const auto& landmarks = sfmData.getLandmarks();

        for (const auto& [_, landmark] : landmarks)
        {
            for (const auto& [viewId, observation] : landmark.getObservations())
            {
                const auto featureDatasIt = _mreconstruction.featureDatasPerView.find(viewId);
                if (featureDatasIt == _mreconstruction.featureDatasPerView.end())
                {
                    continue;
                }

                auto& featureDatas = featureDatasIt->second;
                if (observation.getFeatureId() >= featureDatas.size())
                {
                    continue;
                }

                auto& data = featureDatas[observation.getFeatureId()];

                data.hasLandmark = true;

                if (!sfmData.getViews().count(viewId))
                {
                    continue;
                }
                const auto& view = sfmData.getView(static_cast<aliceVision::IndexT>(viewId));
                if (!sfmData.isPoseAndIntrinsicDefined(&view))
                {
                    continue;
                }
                const auto& pose = sfmData.getPose(view);
                const auto& camTransform = pose.getTransform();
                const auto& intrinsic = sfmData.getIntrinsicPtr(view.getIntrinsicId());
                const aliceVision::Vec2 reprojection = intrinsic->project(camTransform, landmark.X.homogeneous());
                data.rx = static_cast<float>(reprojection.x());
                data.ry = static_cast<float>(reprojection.y());
            }
        }
    }

    // tracks

    if (validTracks)
    {
        const auto& tracks = _mtracks->tracks();

        for (const auto& [_, track] : tracks)
        {
            if (track.featPerView.size() < 2)
            {
                continue;
            }

            MReconstruction::TrackData trackData;

            trackData.averageScale = 0.f;
            trackData.nbReconstructed = 0;

            for (const auto& [viewId, trackItem] : track.featPerView)
            {
                const auto featureDatasIt = _mreconstruction.featureDatasPerView.find(static_cast<aliceVision::IndexT>(viewId));
                if (featureDatasIt == _mreconstruction.featureDatasPerView.end())
                {
                    continue;
                }

                size_t featureId = trackItem.featureId;

                auto& featureDatas = featureDatasIt->second;
                if (featureId >= featureDatas.size())
                {
                    continue;
                }

                auto& data = featureDatas[featureId];

                data.hasTrack = true;

                trackData.averageScale += data.scale;

                if (validLandmarks)
                {
                    MReconstruction::PointwiseTrackData elt;
                    elt.viewId = static_cast<aliceVision::IndexT>(viewId);
                    elt.featureId = static_cast<aliceVision::IndexT>(featureId);

                    const auto& sfmData = _msfmdata->rawData();
                    if (sfmData.getViews().count(viewId))
                    {
                        const auto& view = sfmData.getView(static_cast<aliceVision::IndexT>(viewId));
                        elt.frameId = view.getFrameId();
                    }

                    trackData.elements.push_back(elt);

                    if (data.hasLandmark)
                    {
                        trackData.nbReconstructed++;
                    }
                }
            }

            trackData.averageScale /= static_cast<float>(track.featPerView.size());

            std::sort(
              trackData.elements.begin(), trackData.elements.end(), [](const auto& elt1, const auto& elt2) { return elt1.frameId < elt2.frameId; });

            _mreconstruction.trackDatas.push_back(trackData);
        }
    }

    // signal

    Q_EMIT reconstructionChanged();
}

}  // namespace qtAliceVision
