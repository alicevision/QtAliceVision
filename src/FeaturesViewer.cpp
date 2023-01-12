#include "FeaturesViewer.hpp"

#include <QSGGeometryNode>
#include <QSGGeometry>
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

  void FeaturesViewer::updatePaintFeatures(const PaintParams& params, QSGNode* oldNode, QSGNode* node)
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
      kFeatVertices = (4 * 2) + 2;  // doubled rectangle points + orientation line
      break;
    default:
      break;
    }

    if (!oldNode || oldNode->childCount() == 0)
    {
      auto root = new QSGGeometryNode;
      {
        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;
        geometry = new QSGGeometry(
          QSGGeometry::defaultAttributes_ColoredPoint2D(),
          static_cast<int>(params.nbFeaturesToDraw * kFeatVertices),
          static_cast<int>(params.nbFeaturesToDraw * kFeatIndices),
          QSGGeometry::UnsignedIntType);

        geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
        geometry->setVertexDataPattern(QSGGeometry::StaticPattern);

        root->setGeometry(geometry);
        root->setFlags(QSGNode::OwnsGeometry);
        root->setFlags(QSGNode::OwnsMaterial);
        root->setMaterial(material);
      }
      node->appendChildNode(root);
    }
    else
    {
      if (oldNode->childCount() < 1)
        return;
      auto* root = static_cast<QSGGeometryNode*>(oldNode->firstChild());
      if (!oldNode)
        return;
      root->markDirty(QSGNode::DirtyGeometry);
      geometry = root->geometry();
      geometry->allocate(
        static_cast<int>(params.nbFeaturesToDraw * kFeatVertices),
        static_cast<int>(params.nbFeaturesToDraw * kFeatIndices)
      );
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

    // utility lambda to register a vertex
    const auto setVertice = [&](unsigned int index, const QPointF& point)
    {
      QColor c = _featureColor;
      vertices[index].set(
        static_cast<float>(point.x()), static_cast<float>(point.y()),
        static_cast<uchar>(c.red()), static_cast<uchar>(c.green()), static_cast<uchar>(c.blue()), static_cast<uchar>(c.alpha())
      );
    };

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
        qWarning() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " features, Error on number of features.";
        break;
      }

      const auto& feat = feature->pointFeature();
      const float radius = feat.scale();
      const double diag = 2.0 * feat.scale();
      unsigned int vidx = nbFeaturesDrawn * kFeatVertices;
      unsigned int iidx = nbFeaturesDrawn * kFeatIndices;

      if (_featureDisplayMode == FeaturesViewer::Points)
      {
        setVertice(vidx, QPointF(feat.x(), feat.y()));
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
          setVertice(vidx, tl);
          setVertice(vidx + 1, tr);
          setVertice(vidx + 2, br);
          setVertice(vidx + 3, bl);
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
            setVertice(vidx + lidx, points[k]);
            setVertice(vidx + lidx + 1, points[k + 1]);
          }
          // orientation line: up vector (0, 1)
          const auto nbPoints = static_cast<unsigned int>(points.size());
          setVertice(vidx + nbPoints * 2 - 2, rect.center());
          auto o2 = t.map(rect.center() - QPointF(0.0f, radius)); // rotate end point
          setVertice(vidx + nbPoints * 2 - 1, o2);
        }
      }
      ++nbFeaturesDrawn;
    }
  }

  void FeaturesViewer::updatePaintTracks(const PaintParams& params, QSGNode* oldNode, QSGNode* node)
  {
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " tracks.";

    const unsigned int kLineVertices = 2;

    const MFeatures::MTrackFeaturesPerTrack* trackFeaturesPerTrack = (_displayTracks && params.haveValidFeatures && params.haveValidTracks && params.haveValidLandmarks) ? _mfeatures->getTrackFeaturesPerTrack(_describerType) : nullptr;
    const aliceVision::IndexT currentFrameId = (trackFeaturesPerTrack != nullptr) ? _mfeatures->getCurrentFrameId() : aliceVision::UndefinedIndexT;

    std::size_t nbTracksToDraw = 0;
    std::size_t nbTrackLinesToDraw = 0;
    std::size_t nbReprojectionErrorLinesToDraw = 0;
    std::size_t nbPointsToDraw = 0;
    std::size_t nbHighlightPointsToDraw = 0;

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
        nbTrackLinesToDraw += (trackFeatures.featuresPerFrame.size() - 1); // number of lines in the track
        
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
      }
    }

    QSGGeometry* geometryHighlightPoint = nullptr;
    QSGGeometry* geometryTrackLine = nullptr;
    QSGGeometry* geometryReprojectionErrorLine = nullptr;
    QSGGeometry* geometryPoint = nullptr;

    if (!oldNode || oldNode->childCount() < 5)
    {
      // (1) Highlight points
      {
        auto root = new QSGGeometryNode;
        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;
        {
          geometryHighlightPoint = new QSGGeometry(
            QSGGeometry::defaultAttributes_ColoredPoint2D(),
            static_cast<int>(nbHighlightPointsToDraw),
            static_cast<int>(0),
            QSGGeometry::UnsignedIntType);

          geometryHighlightPoint->setIndexDataPattern(QSGGeometry::StaticPattern);
          geometryHighlightPoint->setVertexDataPattern(QSGGeometry::StaticPattern);

          root->setGeometry(geometryHighlightPoint);
          root->setFlags(QSGNode::OwnsGeometry);
          root->setFlags(QSGNode::OwnsMaterial);
          root->setMaterial(material);
        }
        node->appendChildNode(root);
      }
      // (2) Track lines
      {
        auto root = new QSGGeometryNode;

        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;

        geometryTrackLine = new QSGGeometry(
          QSGGeometry::defaultAttributes_ColoredPoint2D(),
          static_cast<int>(nbTrackLinesToDraw * kLineVertices),
          static_cast<int>(0),
          QSGGeometry::UnsignedIntType);

        geometryTrackLine->setIndexDataPattern(QSGGeometry::StaticPattern);
        geometryTrackLine->setVertexDataPattern(QSGGeometry::StaticPattern);

        root->setGeometry(geometryTrackLine);
        root->setFlags(QSGNode::OwnsGeometry);
        root->setFlags(QSGNode::OwnsMaterial);
        root->setMaterial(material);

        node->appendChildNode(root);
      }
      // (3) Reprojection Error lines
      {
        auto root = new QSGGeometryNode;

        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;

        geometryReprojectionErrorLine = new QSGGeometry(
          QSGGeometry::defaultAttributes_ColoredPoint2D(),
          static_cast<int>(nbReprojectionErrorLinesToDraw * kLineVertices),
          static_cast<int>(0),
          QSGGeometry::UnsignedIntType);

        geometryReprojectionErrorLine->setIndexDataPattern(QSGGeometry::StaticPattern);
        geometryReprojectionErrorLine->setVertexDataPattern(QSGGeometry::StaticPattern);

        root->setGeometry(geometryReprojectionErrorLine);
        root->setFlags(QSGNode::OwnsGeometry);
        root->setFlags(QSGNode::OwnsMaterial);
        root->setMaterial(material);

        node->appendChildNode(root);
      }
      // (4) Points
      {
        auto root = new QSGGeometryNode;
        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;
        {
          geometryPoint = new QSGGeometry(
            QSGGeometry::defaultAttributes_ColoredPoint2D(),
            static_cast<int>(nbPointsToDraw),
            static_cast<int>(0),
            QSGGeometry::UnsignedIntType);

          geometryPoint->setIndexDataPattern(QSGGeometry::StaticPattern);
          geometryPoint->setVertexDataPattern(QSGGeometry::StaticPattern);

          root->setGeometry(geometryPoint);
          root->setFlags(QSGNode::OwnsGeometry);
          root->setFlags(QSGNode::OwnsMaterial);
          root->setMaterial(material);
        }
        node->appendChildNode(root);
      }
    }
    else
    {
      if (oldNode->childCount() < 5)
        return;

      auto* rootHighlightPoint = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(1));
      auto* rootTrackLine = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(2));
      auto* rootReprojectionErrorLine = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(3));
      auto* rootPoint = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(4));

      if (!rootHighlightPoint || !rootTrackLine || !rootReprojectionErrorLine || !rootPoint)
        return;
      
      rootHighlightPoint->markDirty(QSGNode::DirtyGeometry);
      rootTrackLine->markDirty(QSGNode::DirtyGeometry);
      rootReprojectionErrorLine->markDirty(QSGNode::DirtyGeometry);
      rootPoint->markDirty(QSGNode::DirtyGeometry);

      // (1) Highlight points
      geometryHighlightPoint = rootHighlightPoint->geometry();
      geometryHighlightPoint->allocate(
        static_cast<int>(nbHighlightPointsToDraw),
        static_cast<int>(0)
      );
      // (2) Tracks lines
      geometryTrackLine = rootTrackLine->geometry();
      geometryTrackLine->allocate(
        static_cast<int>(nbTrackLinesToDraw * kLineVertices),
        static_cast<int>(0)
      );
      // (3) Reprojection Error lines
      geometryReprojectionErrorLine = rootReprojectionErrorLine->geometry();
      geometryReprojectionErrorLine->allocate(
        static_cast<int>(nbReprojectionErrorLinesToDraw * kLineVertices),
        static_cast<int>(0)
      );
      // (4) Points
      geometryPoint = rootPoint->geometry();
      geometryPoint->allocate(
        static_cast<int>(nbPointsToDraw),
        static_cast<int>(0)
      );
    }

    geometryHighlightPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryHighlightPoint->setLineWidth(6.0f);

    geometryTrackLine->setDrawingMode(QSGGeometry::DrawLines);
    geometryTrackLine->setLineWidth(2.0f);

    geometryReprojectionErrorLine->setDrawingMode(QSGGeometry::DrawLines);
    geometryReprojectionErrorLine->setLineWidth(1.0f);

    geometryPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryPoint->setLineWidth(4.0f);

    QSGGeometry::ColoredPoint2D* verticesHighlightPoints = geometryHighlightPoint->vertexDataAsColoredPoint2D();
    QSGGeometry::ColoredPoint2D* verticesTrackLines = geometryTrackLine->vertexDataAsColoredPoint2D();
    QSGGeometry::ColoredPoint2D* verticesReprojectionErrorLines = geometryReprojectionErrorLine->vertexDataAsColoredPoint2D();
    QSGGeometry::ColoredPoint2D* verticesPoints = geometryPoint->vertexDataAsColoredPoint2D();

    // utility lambda to get line / point color
    const auto getColor = [&](bool colorizeNonContiguous, bool contiguous, bool inliers, bool trackHasInliers) -> QColor
    {
      if (_trackContiguousFilter && !contiguous)
        return QColor(0, 0, 0, 0);

      if (_trackInliersFilter && !trackHasInliers)
        return QColor(0, 0, 0, 0);

      if (colorizeNonContiguous & !contiguous)
        return QColor(50, 50, 50);

      return inliers ? _landmarkColor : _matchColor;
    };

    // utility lambda to register a highlight point vertex
    const auto setVerticeHighlightPoint = [&](unsigned int index, const QPointF& point, int alpha)
    {
      QColor c = QColor(200, 200, 200);
      if (alpha == 0)
        c = QColor(0, 0, 0, 0); // color should be rgba(0,0,0,0) in order to be transparent.
      verticesHighlightPoints[index].set(static_cast<float>(point.x()), static_cast<float>(point.y()), static_cast<uchar>(c.red()), static_cast<uchar>(c.green()), static_cast<uchar>(c.blue()), static_cast<uchar>(c.alpha()));
    };

    // utility lambda to register a track line vertex
    const auto setVerticeTrackLine = [&](unsigned int index, const QPointF& point, const QColor& c)
    {
      verticesTrackLines[index].set(static_cast<float>(point.x()), static_cast<float>(point.y()), static_cast<uchar>(c.red()), static_cast<uchar>(c.green()), static_cast<uchar>(c.blue()), static_cast<uchar>(c.alpha()));
    };

    // utility lambda to register a reprojection error line vertex
    const auto setVerticeReprojectionErrorLine = [&](unsigned int index, const QPointF& point, const QColor& c)
    {
      const QColor rc = c.darker(150); // darken the color to avoid confusion with track lines
      verticesReprojectionErrorLines[index].set(static_cast<float>(point.x()), static_cast<float>(point.y()), static_cast<uchar>(rc.red()), static_cast<uchar>(rc.green()), static_cast<uchar>(rc.blue()), static_cast<uchar>(rc.alpha()));
    };

    // utility lambda to register a point vertex
    const auto setVerticePoint = [&](unsigned int index, const QPointF& point, const QColor& c)
    {
      verticesPoints[index].set(static_cast<float>(point.x()), static_cast<float>(point.y()), static_cast<uchar>(c.red()), static_cast<uchar>(c.green()), static_cast<uchar>(c.blue()), static_cast<uchar>(c.alpha()));
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
       
        setVerticePoint(nbPointsDrawn, trackHasInliers ? point3d : point2d, color);
        ++nbPointsDrawn;

        // draw a highlight point in order to identify the current match from the others
        if (frameId == curFrameId)
        {
          setVerticeHighlightPoint(nbHighlightPointsDrawn, (_display3dTracks && trackHasInliers) ? point3d : point2d, color.alpha());
          ++nbHighlightPointsDrawn;
        }

        // draw reprojection error for landmark
        if (trackHasInliers)
        {
          const unsigned vIdx = nbReprojectionErrorLinesDrawn * kLineVertices;
          setVerticeReprojectionErrorLine(vIdx, point2d, color);
          setVerticeReprojectionErrorLine(vIdx + 1, point3d, color);
          ++nbReprojectionErrorLinesDrawn;
        }
      }
    };

    if (nbTrackLinesToDraw == 0)
      return;

    if (currentFrameId == aliceVision::UndefinedIndexT)
    {
      qInfo() << "[QtAliceVision] FeaturesViewer: Unable to update paint " << _describerType << " tracks, can't find current frame id.";
      return;
    }

    unsigned int nbHighlightPointsDrawn = 0;
    unsigned int nbTrackLinesDrawn = 0;
    unsigned int nbReprojectionErrorLinesDrawn = 0;
    unsigned int nbPointsDrawn = 0;

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

      const bool trackHasInliers = (trackFeatures.nbLandmarks > 0);

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
          const QColor& previousPointColor = getColor(false, contiguous || previousTrackLineContiguous, previousFeatureInlier, trackHasInliers);
          drawFeaturePoint(currentFrameId, previousFrameId, previousFeature, previousPointColor, nbReprojectionErrorLinesDrawn, nbHighlightPointsDrawn, nbPointsDrawn, trackHasInliers);

          // draw track last point
          if (frameId == trackFeatures.maxFrameId)
            drawFeaturePoint(currentFrameId, frameId, feature, getColor(false, contiguous, currentFeatureInlier, trackHasInliers), nbReprojectionErrorLinesDrawn, nbHighlightPointsDrawn, nbPointsDrawn, trackHasInliers);

          // draw track line
          const QColor&  c = getColor(true, contiguous, inliers, trackHasInliers);
          const unsigned vIdx = nbTrackLinesDrawn * kLineVertices;

          if (_display3dTracks && trackHasInliers) // 3d track line
          {
            setVerticeTrackLine(vIdx, QPointF(previousFeature->rx(), previousFeature->ry()), c);
            setVerticeTrackLine(vIdx + 1, QPointF(feature->rx(), feature->ry()), c);
          }
          else // 2d track line
          {
            setVerticeTrackLine(vIdx, QPointF(previousFeature->x(), previousFeature->y()), c);
            setVerticeTrackLine(vIdx + 1, QPointF(feature->x(), feature->y()), c);
          }

          ++nbTrackLinesDrawn;

          previousTrackLineContiguous = contiguous;
        }

        // frame id is now previous frame id
        previousFrameId = frameId;
        previousFeature = feature;
      }
    }
  }

  void FeaturesViewer::updatePaintMatches(const PaintParams& params, QSGNode* oldNode, QSGNode* node)
  {
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " matches.";

    const unsigned int kMatchesVertices = 1;

    QSGGeometry* geometryPoint = nullptr;

    if (!oldNode || oldNode->childCount() < 6)
    {
      auto root = new QSGGeometryNode;
      {
        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;
        geometryPoint = new QSGGeometry(
          QSGGeometry::defaultAttributes_ColoredPoint2D(),
          static_cast<int>(params.nbMatchesToDraw * kMatchesVertices),
          static_cast<int>(0),
          QSGGeometry::UnsignedIntType);

        geometryPoint->setIndexDataPattern(QSGGeometry::StaticPattern);
        geometryPoint->setVertexDataPattern(QSGGeometry::StaticPattern);

        root->setGeometry(geometryPoint);
        root->setFlags(QSGNode::OwnsGeometry);
        root->setFlags(QSGNode::OwnsMaterial);
        root->setMaterial(material);
      }
      node->appendChildNode(root);
    }
    else
    {
      if (oldNode->childCount() < 6)
        return;
      auto* rootPoint = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(5));
      if (!rootPoint)
        return;
      rootPoint->markDirty(QSGNode::DirtyGeometry);
      geometryPoint = rootPoint->geometry();
      geometryPoint->allocate(
        static_cast<int>(params.nbMatchesToDraw * kMatchesVertices),
        static_cast<int>(0)
      );
    }

    geometryPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryPoint->setLineWidth(6.0f);

    QSGGeometry::ColoredPoint2D* verticesPoints = geometryPoint->vertexDataAsColoredPoint2D();

    const auto setVerticePoint = [&](unsigned int index, const QPointF& point)
    {
      verticesPoints[index].set(
        static_cast<float>(point.x()), static_cast<float>(point.y()),
        static_cast<uchar>(_matchColor.red()), static_cast<uchar>(_matchColor.green()), static_cast<uchar>(_matchColor.blue()), static_cast<uchar>(_matchColor.alpha())
      );
    };

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
          qWarning() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " matches, Error on number of matches.";
          break;
        }

        setVerticePoint(nbMatchesDrawn, QPointF(feature->x(), feature->y()));
        ++nbMatchesDrawn;
      }
    }
  }

  void FeaturesViewer::updatePaintLandmarks(const PaintParams& params, QSGNode* oldNode, QSGNode* node)
  {
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " landmarks.";

    const unsigned int kReprojectionVertices = 2;

    QSGGeometry* geometryLine = nullptr;
    QSGGeometry* geometryPoint = nullptr;

    if (!oldNode || oldNode->childCount() < 8)
    {
      auto root = new QSGGeometryNode;
      {
        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;
        {
          geometryLine = new QSGGeometry(
            QSGGeometry::defaultAttributes_ColoredPoint2D(),
            static_cast<int>(params.nbLandmarksToDraw * kReprojectionVertices),
            static_cast<int>(0),
            QSGGeometry::UnsignedIntType);

          geometryLine->setIndexDataPattern(QSGGeometry::StaticPattern);
          geometryLine->setVertexDataPattern(QSGGeometry::StaticPattern);

          root->setGeometry(geometryLine);
          root->setFlags(QSGNode::OwnsGeometry);
          root->setFlags(QSGNode::OwnsMaterial);
          root->setMaterial(material);
        }
        node->appendChildNode(root);
      }
      {
        root = new QSGGeometryNode;
        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;
        {
          geometryPoint = new QSGGeometry(
            QSGGeometry::defaultAttributes_ColoredPoint2D(),
            static_cast<int>(params.nbLandmarksToDraw),
            static_cast<int>(0),
            QSGGeometry::UnsignedIntType);

          geometryPoint->setIndexDataPattern(QSGGeometry::StaticPattern);
          geometryPoint->setVertexDataPattern(QSGGeometry::StaticPattern);

          root->setGeometry(geometryPoint);
          root->setFlags(QSGNode::OwnsGeometry);
          root->setFlags(QSGNode::OwnsMaterial);
          root->setMaterial(material);
        }
        node->appendChildNode(root);
      }
    }
    else
    {
      if (oldNode->childCount() < 8)
        return;
      auto* rootLine = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(6));
      auto* rootPoint = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(7));
      if (!rootLine || !rootPoint)
        return;

      rootLine->markDirty(QSGNode::DirtyGeometry);
      rootPoint->markDirty(QSGNode::DirtyGeometry);

      geometryLine = rootLine->geometry();
      geometryLine->allocate(
        static_cast<int>(params.nbLandmarksToDraw * kReprojectionVertices),
        static_cast<int>(0)
      );
      geometryPoint = rootPoint->geometry();
      geometryPoint->allocate(
        static_cast<int>(params.nbLandmarksToDraw),
        static_cast<int>(0)
      );
    }

    geometryLine->setDrawingMode(QSGGeometry::DrawLines);
    geometryLine->setLineWidth(2.0f);

    geometryPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryPoint->setLineWidth(6.0f);

    QSGGeometry::ColoredPoint2D* verticesLines = geometryLine->vertexDataAsColoredPoint2D();
    QSGGeometry::ColoredPoint2D* verticesPoints = geometryPoint->vertexDataAsColoredPoint2D();

    // utility lambda to register a vertex
    const auto setVerticeLine = [&](unsigned int index, const QPointF& point, const QColor& color)
    {
      verticesLines[index].set(
        static_cast<float>(point.x()), static_cast<float>(point.y()),
        static_cast<uchar>(color.red()), static_cast<uchar>(color.green()), static_cast<uchar>(color.blue()), static_cast<uchar>(color.alpha())
      );
    };
    const auto setVerticePoint = [&](unsigned int index, const QPointF& point, const QColor& color)
    {
      verticesPoints[index].set(
        static_cast<float>(point.x()), static_cast<float>(point.y()),
        static_cast<uchar>(color.red()), static_cast<uchar>(color.green()), static_cast<uchar>(color.blue()), static_cast<uchar>(color.alpha())
      );
    };

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
          qWarning() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " landmarks, Error on number of landmarks.";
          break;
        }

        const unsigned int vidx = nbLandmarksDrawn * kReprojectionVertices;
        setVerticeLine(vidx, QPointF(feature->x(), feature->y()), reprojectionColor);
        setVerticeLine(vidx + 1, QPointF(feature->rx(), feature->ry()), reprojectionColor);
        setVerticePoint(nbLandmarksDrawn, QPointF(feature->rx(), feature->ry()), _landmarkColor);
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

    QSGNode* node = nullptr;
    if (!oldNode)
    {
      node = new QSGNode;
    }
    else
    {
      node = oldNode;
    }

    updatePaintFeatures(params, oldNode, node);
    updatePaintTracks(params, oldNode, node);
    updatePaintMatches(params, oldNode, node);
    updatePaintLandmarks(params, oldNode, node);

    return node;
  }

} // namespace qtAliceVision
