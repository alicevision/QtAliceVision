#include "FeaturesViewer.hpp"

#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>
#include <QTransform>
#include <QtMath>

#include <vector>
#include <algorithm>

namespace qtAliceVision
{

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
    connect(this, &FeaturesViewer::describerTypeChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::featuresChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::tracksChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::sfmDataChanged, this, &FeaturesViewer::update);
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

    if (!params.haveValidFeatures)
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

    const auto& currentViewFeatures = _mfeatures->getFeatures(_describerType.toStdString(), _currentViewId);
    for (const auto& feature : currentViewFeatures)
    {
        // feature scale filter
        if (feature.scale() > params.maxFeatureScale || feature.scale() < params.minFeatureScale)
        {
            continue;
        }

        points.emplace_back(feature.x(), feature.y());
    }

    painter.drawPoints(node, "features", points, _featureColor, 6.f);
}

void FeaturesViewer::paintFeaturesAsSquares(const PaintParams& params, QSGNode* node)
{
    std::vector<QPointF> points;

    const auto& currentViewFeatures = _mfeatures->getFeatures(_describerType.toStdString(), _currentViewId);
    for (const auto& feature : currentViewFeatures)
    {
        // feature scale filter
        if (feature.scale() > params.maxFeatureScale || feature.scale() < params.minFeatureScale)
        {
            continue;
        }

        const float radius = feature.scale();
        const double diag = 2.0 * feature.scale();

        QRectF rect(feature.x(), feature.y(), diag, diag);
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
    std::vector<QLineF> lines;

    const auto& currentViewFeatures = _mfeatures->getFeatures(_describerType.toStdString(), _currentViewId);
    for (const auto& feature : currentViewFeatures)
    {
        // feature scale filter
        if (feature.scale() > params.maxFeatureScale || feature.scale() < params.minFeatureScale)
        {
            continue;
        }

        const float radius = feature.scale();
        const double diag = 2.0 * feature.scale();

        QRectF rect(feature.x(), feature.y(), diag, diag);
        rect.translate(-radius, -radius);
        QPointF tl = rect.topLeft();
        QPointF tr = rect.topRight();
        QPointF br = rect.bottomRight();
        QPointF bl = rect.bottomLeft();

        // compute angle: use feature angle and remove self rotation
        const auto radAngle = -feature.orientation() - qDegreesToRadians(rotation());
        // generate a QTransform that represent this rotation
        const auto t =
            QTransform().translate(feature.x(), feature.y()).rotateRadians(radAngle).translate(-feature.x(), -feature.y());

        // create lines, each vertice has to be duplicated (A->B, B->C, C->D, D->A) since we use GL_LINES
        std::vector<QPointF> points = {t.map(tl), t.map(tr), t.map(br), t.map(bl), t.map(tl)};
        for (unsigned int k = 0; k < points.size(); ++k)
        {
            lines.emplace_back(points[k], points[(k + 1) % points.size()]);
        }
        // orientation line: up vector (0, 1)
        auto o2 = t.map(rect.center() - QPointF(0.0f, radius)); // rotate end point
        lines.emplace_back(rect.center(), o2);
    }

    painter.drawLines(node, "features", lines, _featureColor, 2.f);
}

void FeaturesViewer::updatePaintTracks(const PaintParams& params, QSGNode* node)
{
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " tracks.";

    // TODO: also support having only valid tracks (i.e no landmarks)
    if  (!params.haveValidFeatures || !params.haveValidTracks || !params.haveValidLandmarks)
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

    aliceVision::IndexT currentFrameId = aliceVision::UndefinedIndexT;
    try
    {
        currentFrameId = _msfmdata->rawData().getView(_currentViewId).getFrameId();
    }
    catch (std::exception& e)
    {
        qWarning() << "[QtAliceVision] FeaturesViewer: Failed to retrieve the current frame id." << "\n" << e.what();
    }

    if (currentFrameId == aliceVision::UndefinedIndexT)
    {
        qInfo() << "[QtAliceVision] FeaturesViewer: Unable to update paint " << _describerType
                << " tracks, can't find current frame id.";
        return;
    }

    std::vector<QPointF> trackEndpoints;
    std::vector<QPointF> highlightPoints;
    std::vector<QLineF> trackLines_reconstruction_none;
    std::vector<QLineF> trackLines_reconstruction_partial_outliers;
    std::vector<QLineF> trackLines_reconstruction_partial_inliers;
    std::vector<QLineF> trackLines_reconstruction_full;
    std::vector<QLineF> trackLines_gaps;
    std::vector<QPointF> trackPoints_outliers;
    std::vector<QPointF> trackPoints_inliers;

    const auto& tracksMap = _mtracks->tracks();

    const auto& sfmData = _msfmdata->rawData();
    const auto& landmarks = sfmData.getLandmarks();

    for (const auto& [trackId, track] : tracksMap)
    {
        // check describer type
        if (_describerType.toStdString() != aliceVision::feature::EImageDescriberType_enumToString(track.descType))
        {
            continue;
        }

        // check that track has at least 2 features
        if (track.featPerView.size() < 2)
        {
            continue;
        }

        // apply scale filter
        float avgScale = 0.f;
        for (const auto& [viewId, featId] : track.featPerView)
        {
            const auto& viewFeatures = _mfeatures->getFeatures(_describerType.toStdString(), viewId);
            avgScale += viewFeatures[featId].scale();
        }
        avgScale /= static_cast<float>(track.featPerView.size());
        if (avgScale < params.minFeatureScale || avgScale > params.maxFeatureScale)
        {
            continue;
        }

        // retrieve track reconstruction state
        bool trackHasInliers, trackFullyReconstructed;
        auto landmarkIt = landmarks.find(trackId);
        if (landmarkIt == landmarks.end())
        {
            trackHasInliers = false;
            trackFullyReconstructed = false;
        }
        else
        {
            trackHasInliers = true;
            const auto& landmark = landmarkIt->second;
            trackFullyReconstructed = (landmark.observations.size() == track.featPerView.size());
        }

        // apply inlier filter
        if (!trackHasInliers && _trackInliersFilter)
        {
            continue;
        }

        // build ordered map of {frameId, viewId} in the track
        std::map<aliceVision::IndexT, aliceVision::IndexT> trackFrames;
        for (const auto& [viewId, _] : track.featPerView)
        {
            aliceVision::IndexT frameId = sfmData.getView(viewId).getFrameId();
            trackFrames[frameId] = viewId;
        }

        // check that track frame interval contains current frame
        const aliceVision::IndexT startFrameId = trackFrames.begin()->first;
        const aliceVision::IndexT endFrameId = trackFrames.rbegin()->first;
        if (currentFrameId < startFrameId || currentFrameId > endFrameId)
        {
            continue;
        }

        // utility variables to store data between iterations
        aliceVision::feature::PointFeature previousFeature;
        aliceVision::IndexT previousFrameId = aliceVision::UndefinedIndexT;
        bool previousFeatureInlier = false;
        aliceVision::Vec2 previousObservationPos;
        
        for (const auto& [frameId, viewId] : trackFrames)
        {
            // check that frameId is in timeWindow if enabled
            if (_enableTimeWindow && (frameId < currentFrameId - _timeWindow || frameId > currentFrameId + _timeWindow))
            {
                continue;
            }

            // retrieve point feature
            const auto& currentViewFeatures = _mfeatures->getFeatures(_describerType.toStdString(), viewId);
            const auto& featId = track.featPerView.at(viewId);
            const auto& feature = currentViewFeatures.at(featId);

            // retrieve whether or not current feature has a corresponding observation
            bool currentFeatureInlier;
            aliceVision::Vec2 currentObservationPos;
            if (trackHasInliers)
            {
                const auto& landmark = landmarkIt->second;
                auto observationIt = landmark.observations.find(viewId);
                currentFeatureInlier = (observationIt != landmark.observations.end());
                if (currentFeatureInlier)
                {
                    const auto& observation = observationIt->second;
                    currentObservationPos = observation.x;
                }
            }
            else
            {
                currentFeatureInlier = false;
            }

            if (previousFrameId != aliceVision::UndefinedIndexT)
            {
                // The 2 features of the track are contiguous
                const bool contiguous = (previousFrameId == (frameId - 1));
                // The 2 features of the track are resectioning inliers
                const bool inliers = previousFeatureInlier && currentFeatureInlier;

                // geometry
                // points
                QPointF prevPoint, curPoint;
                if (_display3dTracks)
                {
                    if (previousFeatureInlier)
                    {
                        prevPoint = QPointF(previousObservationPos.x(), previousObservationPos.y());
                    }
                    else
                    {
                        prevPoint = QPointF(previousFeature.x(), previousFeature.y());
                    }

                    if (currentFeatureInlier)
                    {
                        curPoint = QPointF(currentObservationPos.x(), currentObservationPos.y());
                    }
                    else
                    {
                        curPoint = QPointF(feature.x(), feature.y());
                    }
                }
                else
                {
                    prevPoint = QPointF(previousFeature.x(), previousFeature.y());
                    curPoint = QPointF(feature.x(), feature.y());
                }
                // line between the previous and current point
                const QLineF line = QLineF(prevPoint, curPoint);

                // track line
                if (!contiguous)
                {
                    trackLines_gaps.push_back(line);
                }
                else if (!trackHasInliers && !_trackInliersFilter)
                {
                    trackLines_reconstruction_none.push_back(line);
                }
                else if (!trackFullyReconstructed)
                {
                    if (inliers)
                    {
                        trackLines_reconstruction_partial_inliers.push_back(line);
                    }
                    else
                    {
                        trackLines_reconstruction_partial_outliers.push_back(line);
                    }
                }
                else if (trackFullyReconstructed)
                {
                    trackLines_reconstruction_full.push_back(line);
                }

                // highlight point
                if (_trackDisplayMode == WithAllMatches || _trackDisplayMode == WithCurrentMatches)
                {
                    if (previousFrameId == currentFrameId)
                    {
                        highlightPoints.push_back(prevPoint);
                    }
                    else if (frameId == currentFrameId)
                    {
                        highlightPoints.push_back(curPoint);
                    }
                }

                // track points
                // current point
                if (_trackDisplayMode == WithAllMatches ||
                    (frameId == currentFrameId && _trackDisplayMode == WithCurrentMatches))
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
                    if (frameId == endFrameId)
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
            previousFrameId = frameId;
            previousFeature = feature;
            previousFeatureInlier = currentFeatureInlier;
            previousObservationPos = currentObservationPos;
        }
    }

    painter.drawTriangles(node, "trackEndpoints", trackEndpoints, _endpointColor);
    painter.drawPoints(node, "highlightPoints", highlightPoints, QColor(255, 255, 255), 6.f);
    painter.drawLines(node, "trackLines_reconstruction_none", trackLines_reconstruction_none, _featureColor, 2.f);
    painter.drawLines(node, "trackLines_reconstruction_partial_outliers", trackLines_reconstruction_partial_outliers,
                      _matchColor, 2.f);
    painter.drawLines(node, "trackLines_reconstruction_partial_inliers", trackLines_reconstruction_partial_inliers,
                      _landmarkColor, 2.f);
    painter.drawLines(node, "trackLines_reconstruction_full", trackLines_reconstruction_full, _landmarkColor, 5.f);
    painter.drawLines(node, "trackLines_gaps", trackLines_gaps,
                      _trackContiguousFilter ? QColor(0, 0, 0, 0) : QColor(50, 50, 50), 2.f);
    painter.drawPoints(node, "trackPoints_outliers", trackPoints_outliers, _matchColor, 4.f);
    painter.drawPoints(node, "trackPoints_inliers", trackPoints_inliers, _landmarkColor, 4.f);
}

void FeaturesViewer::updatePaintMatches(const PaintParams& params, QSGNode* node)
{
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " matches.";

    // TODO: also support having only valid tracks (i.e no landmarks)
    if (!params.haveValidFeatures || !params.haveValidTracks || !params.haveValidLandmarks)
    {
        // nothing to draw or something is not ready
        painter.clearLayer(node, "matches");
        return;
    }

    std::vector<QPointF> points;

    const auto& currentViewFeatures = _mfeatures->getFeatures(_describerType.toStdString(), _currentViewId);

    const auto& tracksPerView = _mtracks->tracksPerView();
    const auto& currentViewTracks = tracksPerView.at(_currentViewId);

    const auto& tracksMap = _mtracks->tracks();

    const auto& sfmData = _msfmdata->rawData();
    const auto& landmarks = sfmData.getLandmarks();

    for (const auto& trackId : currentViewTracks)
    {
        const auto& track = tracksMap.at(trackId);

        // check describer type
        if (_describerType.toStdString() != aliceVision::feature::EImageDescriberType_enumToString(track.descType))
        {
            continue;
        }

        auto landmarkIt = landmarks.find(trackId);
        if (landmarkIt != landmarks.end())
        {
            const auto& landmark = landmarkIt->second;
            auto observationIt = landmark.observations.find(_currentViewId);
            if (observationIt != landmark.observations.end())
            {
                continue;
            }
        }

        const auto& featId = track.featPerView.at(_currentViewId);
        const auto& feature = currentViewFeatures.at(featId);

        if (feature.scale() > params.maxFeatureScale || feature.scale() < params.minFeatureScale)
        {
            continue;
        }

        points.emplace_back(feature.x(), feature.y());
    }

    painter.drawPoints(node, "matches", points, _matchColor, 6.f);
}

void FeaturesViewer::updatePaintLandmarks(const PaintParams& params, QSGNode* node)
{
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " landmarks.";

    if  (!params.haveValidFeatures || !params.haveValidTracks || !params.haveValidLandmarks)
    {
        // nothing to draw or something is not ready
        painter.clearLayer(node, "reprojectionErrors");
        painter.clearLayer(node, "landmarks");
        return;
    }

    std::vector<QPointF> points;
    std::vector<QLineF> lines;

    const auto& currentViewFeatures = _mfeatures->getFeatures(_describerType.toStdString(), _currentViewId);

    const auto& tracksPerView = _mtracks->tracksPerView();
    const auto& currentViewTracks = tracksPerView.at(_currentViewId);

    const auto& tracksMap = _mtracks->tracks();

    const auto& sfmData = _msfmdata->rawData();
    const auto& landmarks = sfmData.getLandmarks();

    for (const auto& trackId : currentViewTracks)
    {
        const auto& track = tracksMap.at(trackId);

        // check describerType
        if (_describerType.toStdString() != aliceVision::feature::EImageDescriberType_enumToString(track.descType))
        {
            continue;
        }

        auto landmarkIt = landmarks.find(trackId);
        if (landmarkIt == landmarks.end())
        {
            continue;
        }

        const auto& landmark = landmarkIt->second;

        auto observationIt = landmark.observations.find(_currentViewId);
        if (observationIt == landmark.observations.end())
        {
            continue;
        }

        const auto& observation = observationIt->second;
        const aliceVision::Vec2& reprojection = observation.x;

        const auto& featId = track.featPerView.at(_currentViewId);
        const auto& feature = currentViewFeatures.at(featId);

        if (feature.scale() > params.maxFeatureScale || feature.scale() < params.minFeatureScale)
        {
            continue;
        }

        lines.emplace_back(feature.x(), feature.y(), reprojection.x(), reprojection.y());
        points.emplace_back(reprojection.x(), reprojection.y());
    }

    const QColor reprojectionColor = _landmarkColor.darker(150);
    painter.drawLines(node, "reprojectionErrors", lines, reprojectionColor, 1.f);

    painter.drawPoints(node, "landmarks", points, _landmarkColor, 6.f);
}

void FeaturesViewer::initializePaintParams(PaintParams& params)
{
    params.haveValidFeatures = _mfeatures ?
        _mfeatures->rawDataPtr() != nullptr && _mfeatures->status() == MFeatures::Ready : false;

    if (!params.haveValidFeatures)
        return;

    params.haveValidTracks = _mtracks ?
        _mtracks->tracksPtr() != nullptr && _mtracks->status() == MTracks::Ready : false;
    
    params.haveValidLandmarks = _msfmdata ?
        _msfmdata->rawDataPtr() != nullptr && _msfmdata->status() == MSfMData::Ready : false;

    const float minFeatureScale = _mfeatures->getMinFeatureScale(_describerType.toStdString());
    const float difFeatureScale = _mfeatures->getMaxFeatureScale(_describerType.toStdString()) - minFeatureScale;

    params.minFeatureScale = minFeatureScale + std::max(0.f, std::min(1.f, _featureMinScaleFilter)) * difFeatureScale;
    params.maxFeatureScale = minFeatureScale + std::max(0.f, std::min(1.f, _featureMaxScaleFilter)) * difFeatureScale;
}

QSGNode* FeaturesViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
{
    (void)data; // Fix "unused parameter" warnings; should be replaced by [[maybe_unused]] when C++17 is supported
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

} // namespace qtAliceVision
