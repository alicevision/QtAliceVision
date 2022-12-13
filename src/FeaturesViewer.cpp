#include "FeaturesViewer.hpp"

#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>
#include <QSGFlatColorMaterial>

#include <QtMath>
#include <QTransform>

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

  FeaturesViewer::~FeaturesViewer()
  {}

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

    QSGGeometry* geometry = nullptr;

    unsigned int kFeatVertices = 0;
    unsigned int kFeatIndices = 0;

    switch (_featureDisplayMode)
    {
    case FeaturesViewer::Points:
      kFeatVertices = 1;
      break;
    case FeaturesViewer::Squares:
      kFeatVertices = 4;
      kFeatIndices = 6;
      break;
    case FeaturesViewer::OrientedSquares:
      kFeatVertices = (4 * 2) + 2;  // rectangle edges + orientation line
      break;
    default:
      break;
    }

    int vertexCount = params.nbFeaturesToDraw * kFeatVertices;
    int indexCount = params.nbFeaturesToDraw * kFeatIndices;
    if (node->childCount() < 1)
    {
      geometry = appendChildGeometry(node, vertexCount, indexCount);
    }
    else
    {
      geometry = getCleanChildGeometry(node, 0, vertexCount, indexCount);
    }

    switch (_featureDisplayMode)
    {
    case FeaturesViewer::Points:
      geometry->setDrawingMode(QSGGeometry::DrawPoints);
      geometry->setLineWidth(6.0f);
      break;
    case FeaturesViewer::Squares:
      geometry->setDrawingMode(QSGGeometry::DrawTriangles);
      break;
    case FeaturesViewer::OrientedSquares:
      geometry->setDrawingMode(QSGGeometry::DrawLines);
      geometry->setLineWidth(1.0f);
      break;
    default:
      break;
    }

    QSGGeometry::ColoredPoint2D* vertices = geometry->vertexDataAsColoredPoint2D();
    auto* indices = geometry->indexDataAsUInt();

    if (params.nbFeaturesToDraw == 0) // nothing to draw or something is not ready
      return;

    unsigned int nbFeaturesDrawn = 0;

    const MFeatures::MViewFeatures* currentViewFeatures = _mfeatures->getCurrentViewFeatures(_describerType);

    for (const auto& feature : currentViewFeatures->features)
    {
      // feature scale filter
      if (feature->scale() > params.maxFeatureScale ||
          feature->scale() < params.minFeatureScale)
      {
        continue;
      }

      if (nbFeaturesDrawn >= params.nbFeaturesToDraw)
      {
        qWarning() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType
                   << " features, Error on number of features.";
        break;
      }

      const auto& feat = feature->pointFeature();
      const float radius = feat.scale();
      const double diag = 2.0 * feat.scale();
      unsigned int vidx = nbFeaturesDrawn * kFeatVertices;
      unsigned int iidx = nbFeaturesDrawn * kFeatIndices;

      if (_featureDisplayMode == FeaturesViewer::Points)
      {
        setVertex(vertices, vidx, QPointF(feat.x(), feat.y()), _featureColor);
      }
      else
      {
        QRectF rect(feat.x(), feat.y(), diag, diag);
        rect.translate(-radius, -radius);
        QPointF tl = rect.topLeft();
        QPointF tr = rect.topRight();
        QPointF br = rect.bottomRight();
        QPointF bl = rect.bottomLeft();

        if (_featureDisplayMode == FeaturesViewer::Squares)
        {
          // create 2 triangles
          setVertex(vertices, vidx, tl, _featureColor);
          setVertex(vertices, vidx + 1, tr, _featureColor);
          setVertex(vertices, vidx + 2, br, _featureColor);
          setVertex(vertices, vidx + 3, bl, _featureColor);
          indices[iidx] = vidx;
          indices[iidx + 1] = vidx + 1;
          indices[iidx + 2] = vidx + 2;
          indices[iidx + 3] = vidx + 2;
          indices[iidx + 4] = vidx + 3;
          indices[iidx + 5] = vidx;
        }
        else if (_featureDisplayMode == FeaturesViewer::OrientedSquares)
        {
          // compute angle: use feature angle and remove self rotation
          const auto radAngle = -feat.orientation() - qDegreesToRadians(rotation());
          // generate a QTransform that represent this rotation
          const auto t = QTransform().translate(feat.x(), feat.y()).rotateRadians(radAngle).translate(-feat.x(), -feat.y());

          // create lines, each vertice has to be duplicated (A->B, B->C, C->D, D->A) since we use GL_LINES
          std::vector<QPointF> points = { t.map(tl), t.map(tr), t.map(br), t.map(bl), t.map(tl) };
          for (unsigned int k = 0; k < points.size(); ++k)
          {
            auto lidx = k * 2; // local index
            setVertex(vertices, vidx + lidx, points[k], _featureColor);
            setVertex(vertices, vidx + lidx + 1, points[k + 1], _featureColor);
          }
          // orientation line: up vector (0, 1)
          const auto nbPoints = static_cast<unsigned int>(points.size());
          setVertex(vertices, vidx + nbPoints * 2 - 2, rect.center(), _featureColor);
          auto o2 = t.map(rect.center() - QPointF(0.0f, radius)); // rotate end point
          setVertex(vertices, vidx + nbPoints * 2 - 1, o2, _featureColor);
        }
      }
      ++nbFeaturesDrawn;
    }
  }

  void FeaturesViewer::updatePaintTracks(const PaintParams& params, QSGNode* node)
  {
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " tracks.";

    const unsigned int kLineVertices = 2;
    const unsigned int kTriangleVertices = 3;

    const MFeatures::MTrackFeaturesPerTrack* trackFeaturesPerTrack =
        (_displayTracks && params.haveValidFeatures && params.haveValidTracks && params.haveValidLandmarks)
        ? _mfeatures->getTrackFeaturesPerTrack(_describerType) : nullptr;
    const aliceVision::IndexT currentFrameId = (trackFeaturesPerTrack != nullptr)
        ? _mfeatures->getCurrentFrameId() : aliceVision::UndefinedIndexT;

    std::size_t nbTracksToDraw = 0;
    std::size_t nbTrackLinesToDraw[3] = {0, 0, 0};
    std::size_t nbReprojectionErrorLinesToDraw = 0;
    std::size_t nbPointsToDraw = 0;
    std::size_t nbHighlightPointsToDraw = 0;
    std::size_t nbEndpointsToDraw = 0;

    if (trackFeaturesPerTrack != nullptr)
    {
      for (const auto& trackFeaturesPair : *trackFeaturesPerTrack)
      {
        const auto& trackFeatures = trackFeaturesPair.second;

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

        ++nbTracksToDraw;

        const auto& globalTrackInfo = _mfeatures->globalTrackInfo(trackFeaturesPair.first);
        const int state = globalTrackInfo.reconstructionState();
        nbTrackLinesToDraw[state] += (trackFeatures.featuresPerFrame.size() - 1); // number of lines in the track
        
        if (_trackDisplayMode == WithCurrentMatches)
        {
          const auto it = trackFeatures.featuresPerFrame.find(currentFrameId);
          if (it != trackFeatures.featuresPerFrame.end())
          {
            if (trackFeatures.nbLandmarks > 0)
              ++nbReprojectionErrorLinesToDraw; // to draw rerojection error
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
            nbReprojectionErrorLinesToDraw += trackFeatures.featuresPerFrame.size(); // one line per matches for rerojection error

          nbPointsToDraw += trackFeatures.featuresPerFrame.size(); // one point per matches
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

    QSGGeometry* geometryHighlightPoint = nullptr;
    QSGGeometry* geometryTrackLine[3] = {nullptr, nullptr, nullptr};
    QSGGeometry* geometryReprojectionErrorLine = nullptr;
    QSGGeometry* geometryPoint = nullptr;
    QSGGeometry* geometryEndpoint = nullptr;

    if (node->childCount() < 8)
    {
      // (1) Endpoints
      geometryEndpoint = appendChildGeometry(node, nbEndpointsToDraw);
      // (2) Highlight points
      geometryHighlightPoint = appendChildGeometry(node, nbHighlightPointsToDraw);
      // (3) Track lines
      for (std::size_t idx = 0; idx < 3; idx++)
        geometryTrackLine[idx] = appendChildGeometry(node, nbTrackLinesToDraw[idx] * kLineVertices);
      // (4) Reprojection Error lines
      geometryReprojectionErrorLine = appendChildGeometry(node, nbReprojectionErrorLinesToDraw * kLineVertices);
      // (5) Points
      geometryPoint = appendChildGeometry(node, nbPointsToDraw);
    }
    else
    {
      // (1) Endpoints
      geometryEndpoint = getCleanChildGeometry(node, 1, nbEndpointsToDraw * kTriangleVertices);
      // (2) Highlight points
      geometryHighlightPoint = getCleanChildGeometry(node, 2, nbHighlightPointsToDraw);
      // (3) Tracks lines
      for (std::size_t idx = 0; idx < 3; idx++) 
        geometryTrackLine[idx] = getCleanChildGeometry(node, idx+3, nbTrackLinesToDraw[idx] * kLineVertices);
      // (4) Reprojection Error lines
      geometryReprojectionErrorLine = getCleanChildGeometry(node, 6, nbReprojectionErrorLinesToDraw * kLineVertices);
      // (5) Points
      geometryPoint = getCleanChildGeometry(node, 7, nbPointsToDraw);
    }

    geometryHighlightPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryHighlightPoint->setLineWidth(6.0f);

    geometryTrackLine[0]->setDrawingMode(QSGGeometry::DrawLines);
    geometryTrackLine[0]->setLineWidth(2.0f);

    geometryTrackLine[1]->setDrawingMode(QSGGeometry::DrawLines);
    geometryTrackLine[1]->setLineWidth(2.0f);

    geometryTrackLine[2]->setDrawingMode(QSGGeometry::DrawLines);
    geometryTrackLine[2]->setLineWidth(5.0f);

    geometryReprojectionErrorLine->setDrawingMode(QSGGeometry::DrawLines);
    geometryReprojectionErrorLine->setLineWidth(1.0f);

    geometryPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryPoint->setLineWidth(4.0f);

    geometryEndpoint->setDrawingMode(QSGGeometry::DrawTriangles);

    QSGGeometry::ColoredPoint2D* verticesHighlightPoints = geometryHighlightPoint->vertexDataAsColoredPoint2D();
    QSGGeometry::ColoredPoint2D* verticesTrackLines[3] = {
      geometryTrackLine[0]->vertexDataAsColoredPoint2D(), 
      geometryTrackLine[1]->vertexDataAsColoredPoint2D(), 
      geometryTrackLine[2]->vertexDataAsColoredPoint2D()
    };
    QSGGeometry::ColoredPoint2D* verticesReprojectionErrorLines = geometryReprojectionErrorLine->vertexDataAsColoredPoint2D();
    QSGGeometry::ColoredPoint2D* verticesPoints = geometryPoint->vertexDataAsColoredPoint2D();
    QSGGeometry::ColoredPoint2D* verticesEndpoints = geometryEndpoint->vertexDataAsColoredPoint2D();

    // utility lambda to get point color
    const auto getPointColor = [&](bool contiguous, bool inliers, bool trackHasInliers) -> QColor
    {
      if (_trackContiguousFilter && !contiguous)
        return QColor(0, 0, 0, 0);

      if (_trackInliersFilter && !trackHasInliers)
        return QColor(0, 0, 0, 0);

      return inliers ? _landmarkColor : _matchColor;
    };

    // utility lambda to get line color
    const auto getLineColor = [&](bool contiguous, bool inliers, bool trackHasInliers) -> QColor
    {
      if (_trackContiguousFilter && !contiguous)
        return QColor(0, 0, 0, 0);

      if (_trackInliersFilter && !trackHasInliers)
        return QColor(0, 0, 0, 0);

      if (!contiguous)
        return QColor(50, 50, 50);
      
      if (!trackHasInliers)
        return _featureColor;

      return inliers ? _landmarkColor : _matchColor;
    };

    // utility lambda to register a feature point, to avoid code complexity
    const auto drawFeaturePoint = [&](aliceVision::IndexT curFrameId,
                                      aliceVision::IndexT frameId,
                                      const MFeature* feature,
                                      const QColor& color,
                                      unsigned int& nbReprojectionErrorLinesDrawn,
                                      unsigned int& nbHighlightPointsDrawn,
                                      unsigned int& nbPointsDrawn,
                                      bool trackHasInliers)
    {
      if (_trackDisplayMode == WithAllMatches || (frameId == curFrameId && _trackDisplayMode == WithCurrentMatches))
      {
        const QPointF point2d = QPointF(feature->x(), feature->y());
        const QPointF point3d = QPointF(feature->rx(), feature->ry());
        setVertex(verticesPoints, nbPointsDrawn, trackHasInliers ? point3d : point2d, color);
        ++nbPointsDrawn;

        // draw a highlight point in order to identify the current match from the others
        if (frameId == curFrameId)
        {
          QColor colorHighlight = QColor(200, 200, 200);
          if (color.alpha() == 0)
            colorHighlight = QColor(0, 0, 0, 0); // color should be rgba(0,0,0,0) in order to be transparent.
          setVertex(verticesHighlightPoints, nbHighlightPointsDrawn, (_display3dTracks && trackHasInliers) ? point3d : point2d, colorHighlight);
          ++nbHighlightPointsDrawn;
        }

        // draw reprojection error for landmark
        if (trackHasInliers)
        {
          const unsigned vIdx = nbReprojectionErrorLinesDrawn * kLineVertices;
          setVertex(verticesReprojectionErrorLines, vIdx, point2d, color);
          setVertex(verticesReprojectionErrorLines, vIdx + 1, point3d, color);
          ++nbReprojectionErrorLinesDrawn;
        }
      }
    };

    if (nbTracksToDraw == 0)
      return;

    if (currentFrameId == aliceVision::UndefinedIndexT)
    {
      qInfo() << "[QtAliceVision] FeaturesViewer: Unable to update paint "
              << _describerType << " tracks, can't find current frame id.";
      return;
    }

    unsigned int nbHighlightPointsDrawn = 0;
    unsigned int nbTrackLinesDrawn[3] = {0, 0, 0};
    unsigned int nbReprojectionErrorLinesDrawn = 0;
    unsigned int nbPointsDrawn = 0;
    unsigned int nbEndpointsDrawn = 0;

    for (const auto& trackFeaturesPair : *trackFeaturesPerTrack)
    {
      const auto& trackFeatures = trackFeaturesPair.second;

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

      const auto& globalTrackInfo = _mfeatures->globalTrackInfo(trackFeaturesPair.first);
      const int state = globalTrackInfo.reconstructionState();
      const bool trackHasInliers = (state != 0);

      const MFeature* previousFeature = nullptr;
      aliceVision::IndexT previousFrameId = aliceVision::UndefinedIndexT;
      bool previousTrackLineContiguous = false;

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

          // draw previous point
          const QColor& previousPointColor = getPointColor(contiguous || previousTrackLineContiguous, previousFeatureInlier, trackHasInliers);
          drawFeaturePoint(currentFrameId, previousFrameId, previousFeature, previousPointColor, nbReprojectionErrorLinesDrawn, nbHighlightPointsDrawn, nbPointsDrawn, trackFeatures.nbLandmarks > 0);

          // draw track last point
          if (frameId == trackFeatures.maxFrameId)
            drawFeaturePoint(currentFrameId, frameId, feature, getPointColor(contiguous, currentFeatureInlier, trackHasInliers), nbReprojectionErrorLinesDrawn, nbHighlightPointsDrawn, nbPointsDrawn, trackFeatures.nbLandmarks > 0);

          // draw track line
          const QColor&  c = getLineColor(contiguous, inliers, trackHasInliers);
          unsigned int vIdx = nbTrackLinesDrawn[state] * kLineVertices;

          QPointF prevPoint;
          QPointF curPoint;
          if (_display3dTracks && trackHasInliers) // 3d track line
          {
            prevPoint = QPointF(previousFeature->rx(), previousFeature->ry());
            curPoint = QPointF(feature->rx(), feature->ry());
          }
          else // 2d track line
          {
            prevPoint = QPointF(previousFeature->x(), previousFeature->y());
            curPoint = QPointF(feature->x(), feature->y());
          }

          setVertex(verticesTrackLines[state], vIdx, prevPoint, c);
          setVertex(verticesTrackLines[state], vIdx + 1, curPoint, c);

          ++nbTrackLinesDrawn[state];

          previousTrackLineContiguous = contiguous;

          if (_displayTrackEndpoints)
          {
            // draw global start point
            if (previousFrameId == globalTrackInfo.startFrameId)
            {
              vIdx = nbEndpointsDrawn * kTriangleVertices;
              QPointF triangle[] = {QPointF(1, 0), QPointF(-1, 1), QPointF(-1, -1)};
              float angle = QLineF(curPoint, prevPoint).angle() - rotation();
              float size = 10.f;
              auto tr = QTransform().rotate(180 - angle).scale(size, size);
              for (int i = 0; i < kTriangleVertices; i++) 
                setVertex(verticesEndpoints, vIdx + i, prevPoint + tr.map(triangle[i]), _endpointColor);
              nbEndpointsDrawn++;
            }

            // draw global end point
            if (frameId == globalTrackInfo.endFrameId)
            {
              vIdx = nbEndpointsDrawn * kTriangleVertices;
              QPointF triangle[] = {QPointF(1, 0), QPointF(-1, 1), QPointF(-1, -1)};
              float angle = QLineF(curPoint, prevPoint).angle() - rotation();
              float size = 10.f;
              auto tr = QTransform().rotate(-angle).scale(size, size);
              for (int i = 0; i < kTriangleVertices; i++) 
                setVertex(verticesEndpoints, vIdx + i, curPoint + tr.map(triangle[i]), _endpointColor);
              nbEndpointsDrawn++;
            }
          }
        }

        // frame id is now previous frame id
        previousFrameId = frameId;
        previousFeature = feature;
      }
    }
  }

  void FeaturesViewer::updatePaintMatches(const PaintParams& params, QSGNode* node)
  {
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " matches.";

    QSGGeometry* geometryPoint = nullptr;

    if (node->childCount() < 9)
    {
      geometryPoint = appendChildGeometry(node, params.nbMatchesToDraw);
    }
    else
    {
      geometryPoint = getCleanChildGeometry(node, 8, params.nbMatchesToDraw);
    }

    geometryPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryPoint->setLineWidth(6.0f);

    QSGGeometry::ColoredPoint2D* verticesPoints = geometryPoint->vertexDataAsColoredPoint2D();

    if (params.nbMatchesToDraw == 0) // nothing to draw or something is not ready
      return;

    unsigned int nbMatchesDrawn = 0;

    const MFeatures::MViewFeatures* currentViewFeatures = _mfeatures->getCurrentViewFeatures(_describerType);

    // Draw points in the center of non validated tracks
    for (const auto& feature : currentViewFeatures->features)
    {
      if (feature->trackId() >= 0 && feature->landmarkId() < 0)
      {
        // feature scale filter
        if (feature->scale() > params.maxFeatureScale ||
            feature->scale() < params.minFeatureScale)
        {
          continue;
        }

        if (nbMatchesDrawn >= params.nbMatchesToDraw)
        {
          qWarning() << "[QtAliceVision] FeaturesViewer: Update paint "
                     << _describerType << " matches, Error on number of matches.";
          break;
        }

        setVertex(verticesPoints, nbMatchesDrawn, QPointF(feature->x(), feature->y()), _matchColor);
        ++nbMatchesDrawn;
      }
    }
  }

  void FeaturesViewer::updatePaintLandmarks(const PaintParams& params, QSGNode* node)
  {
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " landmarks.";

    const unsigned int kReprojectionVertices = 2;

    QSGGeometry* geometryLine = nullptr;
    QSGGeometry* geometryPoint = nullptr;

    if (node->childCount() < 11)
    {
      geometryLine = appendChildGeometry(node, params.nbLandmarksToDraw * kReprojectionVertices);
      geometryPoint = appendChildGeometry(node, params.nbLandmarksToDraw);
    }
    else
    {
      geometryLine = getCleanChildGeometry(node, 9, params.nbLandmarksToDraw * kReprojectionVertices);
      geometryPoint = getCleanChildGeometry(node, 10, params.nbLandmarksToDraw);
    }

    geometryLine->setDrawingMode(QSGGeometry::DrawLines);
    geometryLine->setLineWidth(2.0f);

    geometryPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryPoint->setLineWidth(6.0f);

    QSGGeometry::ColoredPoint2D* verticesLines = geometryLine->vertexDataAsColoredPoint2D();
    QSGGeometry::ColoredPoint2D* verticesPoints = geometryPoint->vertexDataAsColoredPoint2D();

    if (params.nbLandmarksToDraw == 0) // nothing to draw or something is not ready
      return;

    unsigned int nbLandmarksDrawn = 0;

    const MFeatures::MViewFeatures* currentViewFeatures = _mfeatures->getCurrentViewFeatures(_describerType);
    const QColor reprojectionColor = _landmarkColor.darker(150);

    // Draw lines between reprojected points and features extracted
    for (const auto& feature : currentViewFeatures->features)
    {
      if (feature->landmarkId() >= 0) // isReconstructed
      {
        // feature scale filter
        if (feature->scale() > params.maxFeatureScale ||
            feature->scale() < params.minFeatureScale)
        {
          continue;
        }

        if (nbLandmarksDrawn >= params.nbLandmarksToDraw)
        {
          qWarning() << "[QtAliceVision] FeaturesViewer: Update paint "
                     << _describerType << " landmarks, Error on number of landmarks.";
          break;
        }

        const unsigned int vidx = nbLandmarksDrawn * kReprojectionVertices;
        setVertex(verticesLines, vidx, QPointF(feature->x(), feature->y()), reprojectionColor);
        setVertex(verticesLines, vidx + 1, QPointF(feature->rx(), feature->ry()), reprojectionColor);
        setVertex(verticesPoints, nbLandmarksDrawn, QPointF(feature->rx(), feature->ry()), _landmarkColor);
        ++nbLandmarksDrawn;
      }
    }
  }

  void FeaturesViewer::initializePaintParams(PaintParams& params)
  {
    if (_mfeatures == nullptr)
      return;

    params.haveValidFeatures =_mfeatures->haveValidFeatures();

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
      if (feature->scale() > params.maxFeatureScale ||
          feature->scale() < params.minFeatureScale)
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

    if (!_displayMatches || !params.haveValidTracks || !params.haveValidLandmarks)
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

  QSGGeometry* FeaturesViewer::getCleanChildGeometry(QSGNode* node, int childIdx, int vertexCount, int indexCount) 
  {
    auto* root = static_cast<QSGGeometryNode*>(node->childAtIndex(childIdx));
    if (!root) 
      return nullptr;
    
    root->markDirty(QSGNode::DirtyGeometry);

    QSGGeometry* geometry = root->geometry();
    geometry->allocate(vertexCount, indexCount);
    return geometry;
  }

  QSGGeometry* FeaturesViewer::appendChildGeometry(QSGNode* node, int vertexCount, int indexCount) 
  {
    auto root = new QSGGeometryNode;
    // use VertexColorMaterial to later be able to draw selection in another color
    auto material = new QSGVertexColorMaterial;

    QSGGeometry* geometry = new QSGGeometry(
      QSGGeometry::defaultAttributes_ColoredPoint2D(),
      vertexCount,
      indexCount,
      QSGGeometry::UnsignedIntType);

    geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
    geometry->setVertexDataPattern(QSGGeometry::StaticPattern);

    root->setGeometry(geometry);
    root->setFlags(QSGNode::OwnsGeometry);
    root->setFlags(QSGNode::OwnsMaterial);
    root->setMaterial(material);

    node->appendChildNode(root);

    return geometry;
  }

  void FeaturesViewer::setVertex(QSGGeometry::ColoredPoint2D* vertices, unsigned int idx, const QPointF& point, const QColor& c)
  {
    vertices[idx].set(point.x(), point.y(), c.red(), c.green(), c.blue(), c.alpha());
  }

} // namespace qtAliceVision
