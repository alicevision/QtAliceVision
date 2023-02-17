#include "FeaturesViewer.hpp"

#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>

#include <QTransform>
#include <QtMath>

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

    // trigger data repaint events
    connect(this, &FeaturesViewer::describerTypeChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::featuresChanged, this, &FeaturesViewer::update);
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

void FeaturesViewer::updatePaintFeatures(const PaintParams& params, QSGNode* node)
{
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " features.";

    if (params.nbFeaturesToDraw == 0)
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

    const MFeatures::MViewFeatures* currentViewFeatures = _mfeatures->getCurrentViewFeatures(_describerType);
    for (const auto& feature : currentViewFeatures->features)
    {
        // feature scale filter
        if (feature->scale() > params.maxFeatureScale || feature->scale() < params.minFeatureScale)
        {
            continue;
        }

        const auto& feat = feature->pointFeature();
        points.emplace_back(feat.x(), feat.y());
    }

    painter.drawPoints(node, "features", points, _featureColor, 6.f);
}

void FeaturesViewer::paintFeaturesAsSquares(const PaintParams& params, QSGNode* node)
{
    std::vector<QPointF> points;

    const MFeatures::MViewFeatures* currentViewFeatures = _mfeatures->getCurrentViewFeatures(_describerType);
    for (const auto& feature : currentViewFeatures->features)
    {
        // feature scale filter
        if (feature->scale() > params.maxFeatureScale || feature->scale() < params.minFeatureScale)
        {
            continue;
        }

        const auto& feat = feature->pointFeature();
        const float radius = feat.scale();
        const double diag = 2.0 * feat.scale();

        QRectF rect(feat.x(), feat.y(), diag, diag);
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

    const MFeatures::MViewFeatures* currentViewFeatures = _mfeatures->getCurrentViewFeatures(_describerType);
    for (const auto& feature : currentViewFeatures->features)
    {
        // feature scale filter
        if (feature->scale() > params.maxFeatureScale || feature->scale() < params.minFeatureScale)
        {
            continue;
        }

        const auto& feat = feature->pointFeature();
        const float radius = feat.scale();
        const double diag = 2.0 * feat.scale();

        QRectF rect(feat.x(), feat.y(), diag, diag);
        rect.translate(-radius, -radius);
        QPointF tl = rect.topLeft();
        QPointF tr = rect.topRight();
        QPointF br = rect.bottomRight();
        QPointF bl = rect.bottomLeft();

        // compute angle: use feature angle and remove self rotation
        const auto radAngle = -feat.orientation() - qDegreesToRadians(rotation());
        // generate a QTransform that represent this rotation
        const auto t =
            QTransform().translate(feat.x(), feat.y()).rotateRadians(radAngle).translate(-feat.x(), -feat.y());

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

    const MFeatures::MTrackFeaturesPerTrack* trackFeaturesPerTrack =
        (_displayTracks && params.haveValidFeatures && params.haveValidTracks && params.haveValidLandmarks)
            ? _mfeatures->getTrackFeaturesPerTrack(_describerType)
            : nullptr;
    const aliceVision::IndexT currentFrameId =
        (trackFeaturesPerTrack != nullptr) ? _mfeatures->getCurrentFrameId() : aliceVision::UndefinedIndexT;

    int nbTracksToDraw = 0;
    int nbTrackLinesToDraw[3] = {0, 0, 0};
    int nbReprojectionErrorLinesToDraw = 0;
    int nbPointsToDraw = 0;
    int nbHighlightPointsToDraw = 0;
    int nbEndpointsToDraw = 0;

    if (trackFeaturesPerTrack != nullptr)
    {
        for (const auto& trackFeaturesPair : *trackFeaturesPerTrack)
        {
            const auto& trackFeatures = trackFeaturesPair.second;
            const auto& globalTrackInfo = _mfeatures->globalTrackInfo(trackFeaturesPair.first);

            // feature scale filter
            if (trackFeatures.featureScaleAverage > params.maxFeatureScale ||
                trackFeatures.featureScaleAverage < params.minFeatureScale)
            {
                continue;
            }

            // track has at least 2 features
            if (trackFeatures.featuresPerFrame.size() < 2)
            {
                continue;
            }

            // track frame interval contains current frame
            if (currentFrameId < globalTrackInfo.startFrameId || currentFrameId > globalTrackInfo.endFrameId)
            {
                continue;
            }

            ++nbTracksToDraw;

            const MFeatures::ReconstructionState state = globalTrackInfo.reconstructionState();
            const int stateIdx = static_cast<int>(state);
            nbTrackLinesToDraw[stateIdx] +=
                static_cast<int>(trackFeatures.featuresPerFrame.size()) - 1; // number of lines in the track

            if (_trackDisplayMode == WithCurrentMatches)
            {
                const auto it = trackFeatures.featuresPerFrame.find(currentFrameId);
                if (it != trackFeatures.featuresPerFrame.end())
                {
                    if (trackFeatures.nbLandmarks > 0)
                        ++nbReprojectionErrorLinesToDraw; // to draw reprojection error
                    ++nbPointsToDraw;
                    ++nbHighlightPointsToDraw;
                }
            }
            else if (_trackDisplayMode == WithAllMatches)
            {
                const auto it = trackFeatures.featuresPerFrame.find(currentFrameId);
                if (it != trackFeatures.featuresPerFrame.end())
                    ++nbHighlightPointsToDraw; // to draw a highlight point in order to identify the current match

                if (trackFeatures.nbLandmarks > 0)
                    nbReprojectionErrorLinesToDraw += static_cast<int>(
                        trackFeatures.featuresPerFrame.size()); // one line per matches for reprojection error

                nbPointsToDraw += static_cast<int>(trackFeatures.featuresPerFrame.size()); // one point per matches
            }

            if (_displayTrackEndpoints)
            {
                if (trackFeatures.minFrameId == globalTrackInfo.startFrameId)
                    ++nbEndpointsToDraw;

                if (trackFeatures.maxFrameId == globalTrackInfo.endFrameId)
                    ++nbEndpointsToDraw;
            }
        }
    }

    if (nbTrackLinesToDraw == 0 || !params.haveValidTracks || !_displayTracks)
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

    for (const auto& trackFeaturesPair : *trackFeaturesPerTrack)
    {
        const auto& trackFeatures = trackFeaturesPair.second;
        const auto& globalTrackInfo = _mfeatures->globalTrackInfo(trackFeaturesPair.first);

        // feature scale filter
        if (trackFeatures.featureScaleAverage > params.maxFeatureScale ||
            trackFeatures.featureScaleAverage < params.minFeatureScale)
        {
            continue;
        }

        // track has at least 2 features
        if (trackFeatures.featuresPerFrame.size() < 2)
        {
            continue;
        }

        // track frame interval contains current frame
        if (currentFrameId < globalTrackInfo.startFrameId || currentFrameId > globalTrackInfo.endFrameId)
        {
            continue;
        }

        const MFeatures::ReconstructionState state = globalTrackInfo.reconstructionState();
        const bool trackHasInliers = (state != MFeatures::ReconstructionState::None);

        const MFeature* previousFeature = nullptr;
        aliceVision::IndexT previousFrameId = aliceVision::UndefinedIndexT;

        for (aliceVision::IndexT frameId = trackFeatures.minFrameId; frameId <= trackFeatures.maxFrameId; ++frameId)
        {
            auto it = trackFeatures.featuresPerFrame.find(frameId);
            if (it == trackFeatures.featuresPerFrame.end())
                continue;

            const auto& feature = it->second;

            if (previousFrameId != aliceVision::UndefinedIndexT)
            {
                // The 2 features of the track are contiguous
                const bool contiguous = (previousFrameId == (frameId - 1));
                // The previous feature is a resectioning inlier
                const bool previousFeatureInlier = previousFeature->landmarkId() >= 0;
                // The current feature is a resectioning inlier
                const bool currentFeatureInlier = feature->landmarkId() >= 0;
                // The 2 features of the track are resectioning inliers
                const bool inliers = previousFeatureInlier && currentFeatureInlier;

                // geometry
                QPointF prevPoint = (_display3dTracks && trackHasInliers)
                                        ? QPointF(previousFeature->rx(), previousFeature->ry())
                                        : QPointF(previousFeature->x(), previousFeature->y());
                QPointF curPoint = (_display3dTracks && trackHasInliers) ? QPointF(feature->rx(), feature->ry())
                                                                         : QPointF(feature->x(), feature->y());
                QLineF line = QLineF(prevPoint, curPoint);

                // track line
                if (!contiguous)
                {
                    trackLines_gaps.push_back(line);
                }
                else if (state == MFeatures::ReconstructionState::None && !_trackInliersFilter)
                {
                    trackLines_reconstruction_none.push_back(line);
                }
                else if (state == MFeatures::ReconstructionState::Partial)
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
                else if (state == MFeatures::ReconstructionState::Complete)
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
                if ((_trackDisplayMode == WithAllMatches && previousFrameId == trackFeatures.minFrameId) ||
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
                    if (previousFrameId == globalTrackInfo.startFrameId)
                    {
                        QPointF triangle[] = {QPointF(0, 0), QPointF(-2, 1), QPointF(-2, -1)};
                        double angle = QLineF(prevPoint, curPoint).angle() - rotation();
                        auto tr = QTransform().rotate(-angle).scale(10, 10);
                        for (int i = 0; i < 3; ++i)
                            trackEndpoints.push_back(prevPoint + tr.map(triangle[i]));
                    }

                    // draw global end point
                    if (frameId == globalTrackInfo.endFrameId)
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

    if (params.nbMatchesToDraw == 0)
    {
        // nothing to draw or something is not ready
        painter.clearLayer(node, "matches");
        return;
    }

    std::vector<QPointF> points;

    const MFeatures::MViewFeatures* currentViewFeatures = _mfeatures->getCurrentViewFeatures(_describerType);
    for (const auto& feature : currentViewFeatures->features)
    {
        if (feature->trackId() >= 0 && feature->landmarkId() < 0)
        {
            // feature scale filter
            if (feature->scale() > params.maxFeatureScale || feature->scale() < params.minFeatureScale)
            {
                continue;
            }

            points.emplace_back(feature->x(), feature->y());
        }
    }

    painter.drawPoints(node, "matches", points, _matchColor, 6.f);
}

void FeaturesViewer::updatePaintLandmarks(const PaintParams& params, QSGNode* node)
{
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " landmarks.";

    if (params.nbLandmarksToDraw == 0)
    {
        // nothing to draw or something is not ready
        painter.clearLayer(node, "reprojectionErrors");
        painter.clearLayer(node, "landmarks");
        return;
    }

    std::vector<QPointF> points;
    std::vector<QLineF> lines;

    const MFeatures::MViewFeatures* currentViewFeatures = _mfeatures->getCurrentViewFeatures(_describerType);
    for (const auto& feature : currentViewFeatures->features)
    {
        if (feature->landmarkId() >= 0)
        {
            // feature scale filter
            if (feature->scale() > params.maxFeatureScale || feature->scale() < params.minFeatureScale)
            {
                continue;
            }

            lines.emplace_back(feature->x(), feature->y(), feature->rx(), feature->ry());
            points.emplace_back(feature->rx(), feature->ry());
        }
    }

    const QColor reprojectionColor = _landmarkColor.darker(150);
    painter.drawLines(node, "reprojectionErrors", lines, reprojectionColor, 1.f);

    painter.drawPoints(node, "landmarks", points, _landmarkColor, 6.f);
}

void FeaturesViewer::initializePaintParams(PaintParams& params)
{
    if (_mfeatures == nullptr)
        return;

    params.haveValidFeatures = _mfeatures->haveValidFeatures();

    if (!params.haveValidFeatures)
        return;

    params.haveValidTracks = _mfeatures->haveValidTracks();
    params.haveValidLandmarks = _mfeatures->haveValidLandmarks();

    const float minFeatureScale = _mfeatures->getMinFeatureScale(_describerType);
    const float difFeatureScale = _mfeatures->getMaxFeatureScale(_describerType) - minFeatureScale;

    params.minFeatureScale = minFeatureScale + std::max(0.f, std::min(1.f, _featureMinScaleFilter)) * difFeatureScale;
    params.maxFeatureScale = minFeatureScale + std::max(0.f, std::min(1.f, _featureMaxScaleFilter)) * difFeatureScale;

    const MFeatures::MViewFeatures* currentViewFeatures = _mfeatures->getCurrentViewFeatures(_describerType);

    for (const auto& feature : currentViewFeatures->features)
    {
        // feature scale filter
        if (feature->scale() > params.maxFeatureScale || feature->scale() < params.minFeatureScale)
        {
            continue;
        }

        if (feature->trackId() >= 0)
        {
            if (feature->landmarkId() >= 0)
                ++params.nbLandmarksToDraw;
            else
                ++params.nbMatchesToDraw;
        }

        ++params.nbFeaturesToDraw;
    }

    if (!_displayFeatures)
        params.nbFeaturesToDraw = 0;

    if (!_displayMatches || !params.haveValidTracks)
        params.nbMatchesToDraw = 0;

    if (!_displayLandmarks || !params.haveValidLandmarks)
        params.nbLandmarksToDraw = 0;
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
