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

    connect(this, &FeaturesViewer::trackContiguousFilterChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::trackInliersFilterChanged, this, &FeaturesViewer::update);

    connect(this, &FeaturesViewer::trackMinFeatureScaleFilterChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::trackMaxFeatureScaleFilterChanged, this, &FeaturesViewer::update);

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

  void FeaturesViewer::updatePaintFeatures(QSGNode* oldNode, QSGNode* node)
  {
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " features.";

    const bool validFeatures = (_mfeatures != nullptr) && (_mfeatures->haveValidFeatures());

    const MFeatures::MViewFeatures* currentViewFeatures = validFeatures ? _mfeatures->getCurrentViewFeatures(_describerType) : nullptr;
    const std::size_t displayNbFeatures = (_displayFeatures && (currentViewFeatures != nullptr)) ? currentViewFeatures->features.size() : 0;
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
    }

    if (!oldNode || oldNode->childCount() == 0)
    {
      auto root = new QSGGeometryNode;
      {
        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;
        geometry = new QSGGeometry(
          QSGGeometry::defaultAttributes_ColoredPoint2D(),
          static_cast<int>(displayNbFeatures * kFeatVertices),
          static_cast<int>(displayNbFeatures * kFeatIndices),
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
        static_cast<int>(displayNbFeatures * kFeatVertices),
        static_cast<int>(displayNbFeatures * kFeatIndices)
      );
    }

    if (displayNbFeatures == 0)
      return;

    const auto& features = currentViewFeatures->features;

    if (features.empty())
      return; // nothing to do, no features in the current view

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
    }

    QSGGeometry::ColoredPoint2D* vertices = geometry->vertexDataAsColoredPoint2D();
    auto* indices = geometry->indexDataAsUInt();

    // utility lambda to register a vertex
    const auto setVertice = [&](unsigned int index, const QPointF& point, bool isReconstructed)
    {
      QColor c = _featureColor; // (isReconstructed ? _colorReproj : _color);
      vertices[index].set(
        point.x(), point.y(),
        c.red(), c.green(), c.blue(), c.alpha()
      );
    };

    for (int i = 0; i < features.size(); ++i)
    {
      const auto& f = features.at(i);
      bool isReconstructed = f->landmarkId() > 0;
      auto& feat = f->pointFeature();
      const auto radius = feat.scale();
      const auto diag = 2.0 * feat.scale();
      unsigned int vidx = i * kFeatVertices;
      unsigned int iidx = i * kFeatIndices;

      if (_featureDisplayMode == FeaturesViewer::Points)
      {
        setVertice(vidx, QPointF(feat.x(), feat.y()), isReconstructed);
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
          setVertice(vidx, tl, isReconstructed);
          setVertice(vidx + 1, tr, isReconstructed);
          setVertice(vidx + 2, br, isReconstructed);
          setVertice(vidx + 3, bl, isReconstructed);
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
            setVertice(vidx + lidx, points[k], isReconstructed);
            setVertice(vidx + lidx + 1, points[k + 1], isReconstructed);
          }
          // orientation line: up vector (0, 1)
          const auto nbPoints = static_cast<unsigned int>(points.size());
          setVertice(vidx + nbPoints * 2 - 2, rect.center(), isReconstructed);
          auto o2 = t.map(rect.center() - QPointF(0.0f, radius)); // rotate end point
          setVertice(vidx + nbPoints * 2 - 1, o2, isReconstructed);
        }
      }
    }
  }

  void FeaturesViewer::updatePaintTracks(QSGNode* oldNode, QSGNode* node)
  {
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " tracks.";

    const unsigned int kTrackLineVertices = 2;

    const bool validFeatures = (_mfeatures != nullptr) && (_mfeatures->haveValidFeatures());
    const bool validTracks = (_mfeatures != nullptr) && _mfeatures->haveValidTracks();
    const bool validLandmarks = (_mfeatures != nullptr) && _mfeatures->haveValidLandmarks(); // TODO: shouldn't be required, should be optionnal but required for now in order to get frameId (SfMData).

    const MFeatures::MTrackFeaturesPerTrack* trackFeaturesPerTrack = (_displayTracks && validFeatures && validTracks && validLandmarks) ? _mfeatures->getTrackFeaturesPerTrack(_describerType) : nullptr;
    const aliceVision::IndexT currentFrameId = (trackFeaturesPerTrack != nullptr) ? _mfeatures->getCurrentFrameId() : aliceVision::UndefinedIndexT;

    std::size_t nbTracksToDraw = 0;
    std::size_t nbLinesToDraw = 0;
    std::size_t nbPointsToDraw = 0;
    std::size_t nbHighlightPointsToDraw = 0;

    if (trackFeaturesPerTrack != nullptr)
    {
      for (const auto& trackFeaturesPair : *trackFeaturesPerTrack)
      {
        const auto& trackFeatures = trackFeaturesPair.second;

        // feature scale score filter
        if (trackFeatures.featureScaleScore > _trackMaxFeatureScaleFilter ||
            trackFeatures.featureScaleScore < _trackMinFeatureScaleFilter)
        {
          continue;
        }

        // track has at least 2 features
        if (trackFeatures.featuresPerFrame.size() < 2)
        {
          continue;
        }

        ++nbTracksToDraw;
        nbLinesToDraw += (trackFeatures.featuresPerFrame.size() - 1); // number of lines in the track
        
        if (_trackDisplayMode == WithCurrentMatches)
        {
          const auto it = trackFeatures.featuresPerFrame.find(currentFrameId);
          if (it != trackFeatures.featuresPerFrame.end())
          {
            if (trackFeatures.nbLandmarks > 0)
              ++nbLinesToDraw; // to draw rerojection error
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
            nbLinesToDraw += trackFeatures.featuresPerFrame.size(); // one line per matches for rerojection error

          nbPointsToDraw += trackFeatures.featuresPerFrame.size(); // one point per matches
        }
      }
    }

    QSGGeometry* geometryLine = nullptr;
    QSGGeometry* geometryHighlightPoint = nullptr;
    QSGGeometry* geometryPoint = nullptr;

    if (!oldNode || oldNode->childCount() < 4)
    {
      {
        auto root = new QSGGeometryNode;

        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;

        geometryLine = new QSGGeometry(
          QSGGeometry::defaultAttributes_ColoredPoint2D(),
          static_cast<int>(nbLinesToDraw * kTrackLineVertices),
          static_cast<int>(0),
          QSGGeometry::UnsignedIntType);

        geometryLine->setIndexDataPattern(QSGGeometry::StaticPattern);
        geometryLine->setVertexDataPattern(QSGGeometry::StaticPattern);

        root->setGeometry(geometryLine);
        root->setFlags(QSGNode::OwnsGeometry);
        root->setFlags(QSGNode::OwnsMaterial);
        root->setMaterial(material);

        node->appendChildNode(root);
      }
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
      if (oldNode->childCount() < 4)
        return;

      auto* rootLine = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(1));
      auto* rootHighlightPoint = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(2));
      auto* rootPoint = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(3));

      if (!rootHighlightPoint || !rootLine || !rootPoint)
        return;

      rootLine->markDirty(QSGNode::DirtyGeometry);
      rootHighlightPoint->markDirty(QSGNode::DirtyGeometry);
      rootPoint->markDirty(QSGNode::DirtyGeometry);

      geometryLine = rootLine->geometry();
      geometryLine->allocate(
        static_cast<int>(nbLinesToDraw * kTrackLineVertices),
        static_cast<int>(0)
      );
      geometryHighlightPoint = rootHighlightPoint->geometry();
      geometryHighlightPoint->allocate(
        static_cast<int>(nbHighlightPointsToDraw),
        static_cast<int>(0)
      );
      geometryPoint = rootPoint->geometry();
      geometryPoint->allocate(
        static_cast<int>(nbPointsToDraw),
        static_cast<int>(0)
      );
    }

    geometryLine->setDrawingMode(QSGGeometry::DrawLines);
    geometryLine->setLineWidth(2.0f);

    geometryHighlightPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryHighlightPoint->setLineWidth(6.0f);

    geometryPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryPoint->setLineWidth(4.0f);

    QSGGeometry::ColoredPoint2D* verticesLines = geometryLine->vertexDataAsColoredPoint2D();
    QSGGeometry::ColoredPoint2D* verticesHighlightPoints = geometryHighlightPoint->vertexDataAsColoredPoint2D();
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

    // utility lambda to register a line vertex
    const auto setVerticeLine = [&](unsigned int index, const QPointF& point, const QColor& c)
    {
      verticesLines[index].set(point.x(), point.y(), c.red(), c.green(), c.blue(), c.alpha());
    };

    // utility lambda to register a point vertex
    const auto setVerticePoint = [&](unsigned int index, const QPointF& point, const QColor& c)
    {
      verticesPoints[index].set(point.x(), point.y(), c.red(), c.green(), c.blue(), c.alpha());
    };

    // utility lambda to register a highlight point vertex
    const auto setVerticeHighlightPoint = [&](unsigned int index, const QPointF& point, int alpha)
    {
      QColor c = QColor(200, 200, 200);
      if (alpha == 0)
        c = QColor(0, 0, 0, 0); // color should be rgba(0,0,0,0) in order to be transparent.
      verticesHighlightPoints[index].set(point.x(), point.y(), c.red(), c.green(), c.blue(), c.alpha());
    };

    // utility lambda to register a feature point, to avoid code complexity
    const auto drawFeaturePoint = [&](aliceVision::IndexT currentFrameId, 
                                      aliceVision::IndexT frameId, 
                                      const MFeature* feature, 
                                      const QColor& color,
                                      unsigned int& nbLinesDrawn, unsigned int& nbHighlightPointsDrawn, unsigned int& nbPointsDrawn, bool trackHasInliers)
    {
      if (_trackDisplayMode == WithAllMatches || (frameId == currentFrameId && _trackDisplayMode == WithCurrentMatches))
      {
        QPointF point = trackHasInliers ? QPointF(feature->rx(), feature->ry()) : QPointF(feature->x(), feature->y());
       
        setVerticePoint(nbPointsDrawn, point, color);
        ++nbPointsDrawn;

        // draw a highlight point in order to identify the current match from the others
        if (frameId == currentFrameId) 
        {
          setVerticeHighlightPoint(nbHighlightPointsDrawn, point, color.alpha());
          ++nbHighlightPointsDrawn;
        }

        // draw reprojection error for landmark
        if (trackHasInliers)
        {
          const QColor reprojectionColor = color.darker(150);
          const unsigned vIdx = nbLinesDrawn * kTrackLineVertices;
          setVerticeLine(vIdx, QPointF(feature->x(), feature->y()), reprojectionColor);
          setVerticeLine(vIdx + 1, QPointF(feature->rx(), feature->ry()), reprojectionColor);
          ++nbLinesDrawn;
        }
      }
    };

    if (nbLinesToDraw == 0)
      return;

    if (currentFrameId == aliceVision::UndefinedIndexT)
    {
      qInfo() << "[QtAliceVision] FeaturesViewer: Unable to update paint " << _describerType << " tracks, can't find current frame id.";
      return;
    }

    unsigned int nbLinesDrawn = 0;
    unsigned int nbHighlightPointsDrawn = 0;
    unsigned int nbPointsDrawn = 0;

    for (const auto& trackFeaturesPair : *trackFeaturesPerTrack)
    {
      const auto& trackFeatures = trackFeaturesPair.second;

      // feature scale score filter
      if (trackFeatures.featureScaleScore > _trackMaxFeatureScaleFilter ||
        trackFeatures.featureScaleScore < _trackMinFeatureScaleFilter)
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
          drawFeaturePoint(currentFrameId, previousFrameId, previousFeature, previousPointColor, nbLinesDrawn, nbHighlightPointsDrawn, nbPointsDrawn, trackHasInliers);

          // draw track last point
          if (frameId == trackFeatures.maxFrameId)
            drawFeaturePoint(currentFrameId, frameId, feature, getColor(false, contiguous, currentFeatureInlier, trackHasInliers), nbLinesDrawn, nbHighlightPointsDrawn, nbPointsDrawn, trackHasInliers);

          // draw track line
          const QColor&  c = getColor(true, contiguous, inliers, trackHasInliers);
          const unsigned vIdx = nbLinesDrawn * kTrackLineVertices;
          setVerticeLine(vIdx, QPointF(previousFeature->x(), previousFeature->y()), c);
          setVerticeLine(vIdx + 1, QPointF(feature->x(), feature->y()), c);
          ++nbLinesDrawn;

          previousTrackLineContiguous = contiguous;
        }

        // frame id is now previous frame id
        previousFrameId = frameId;
        previousFeature = feature;
      }
    }
  }

  void FeaturesViewer::updatePaintMatches(QSGNode* oldNode, QSGNode* node)
  {
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " matches.";

    const bool validFeatures = (_mfeatures != nullptr) && (_mfeatures->haveValidFeatures());
    const bool validTracks = (_mfeatures != nullptr) && _mfeatures->haveValidTracks();

    const MFeatures::MViewFeatures* currentViewFeatures = (validFeatures && validTracks) ? _mfeatures->getCurrentViewFeatures(_describerType) : nullptr;
    const std::size_t displayNbTracks = (_displayMatches && (currentViewFeatures != nullptr)) ? currentViewFeatures->nbTracks - currentViewFeatures->nbLandmarks : 0;

    const unsigned int kTracksVertices = 1;

    QSGGeometry* geometryPoint = nullptr;
    if (!oldNode || oldNode->childCount() < 5)
    {
      auto root = new QSGGeometryNode;
      {
        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;
        geometryPoint = new QSGGeometry(
          QSGGeometry::defaultAttributes_ColoredPoint2D(),
          static_cast<int>(displayNbTracks * kTracksVertices),
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
      if (oldNode->childCount() < 5)
        return;
      auto* rootPoint = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(4));
      if (!rootPoint)
        return;
      rootPoint->markDirty(QSGNode::DirtyGeometry);
      geometryPoint = rootPoint->geometry();
      geometryPoint->allocate(
        static_cast<int>(displayNbTracks * kTracksVertices),
        static_cast<int>(0)
      );
    }

    if (displayNbTracks == 0)
      return;

    const auto& features = currentViewFeatures->features;

    if (features.empty())
      return; // nothing to do, no features in the current view

    geometryPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryPoint->setLineWidth(6.0f);

    QSGGeometry::ColoredPoint2D* verticesPoints = geometryPoint->vertexDataAsColoredPoint2D();

    const auto setVerticePoint = [&](unsigned int index, const QPointF& point)
    {
      verticesPoints[index].set(
        point.x(), point.y(),
        _matchColor.red(), _matchColor.green(), _matchColor.blue(), _matchColor.alpha()
      );
    };

    // Draw points in the center of non validated tracks
    int obsI = 0;
    for (int i = 0; i < features.size(); ++i)
    {
      const auto& f = features.at(i);

      if (f->trackId() < 0)
        continue;
      if (currentViewFeatures->nbLandmarks > 0 && f->landmarkId() >= 0)
        continue;

      if (obsI >= displayNbTracks)
      {
        qWarning() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " matches, Error on number of tracks.";
        break;
      }
      float x = f->x();
      float y = f->y();

      setVerticePoint(obsI, QPointF(x, y));

      ++obsI;
    }
  }

  void FeaturesViewer::updatePaintLandmarks(QSGNode* oldNode, QSGNode* node)
  {
    qDebug() << "[QtAliceVision] FeaturesViewer: Update paint " << _describerType << " landmarks.";

    const bool validFeatures = (_mfeatures != nullptr) && (_mfeatures->haveValidFeatures());
    const bool validLandmarks = (_mfeatures != nullptr) && _mfeatures->haveValidLandmarks();

    const MFeatures::MViewFeatures* currentViewFeatures = (validFeatures && validLandmarks) ? _mfeatures->getCurrentViewFeatures(_describerType) : nullptr;
    const int displayNbLandmarks = (_displayLandmarks && (currentViewFeatures != nullptr)) ? currentViewFeatures->nbLandmarks : 0;

    const unsigned int kReprojectionVertices = 2;

    QSGGeometry* geometryLine = nullptr;
    QSGGeometry* geometryPoint = nullptr;

    if (!oldNode || oldNode->childCount() < 7)
    {
      auto root = new QSGGeometryNode;
      {
        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;
        {
          geometryLine = new QSGGeometry(
            QSGGeometry::defaultAttributes_ColoredPoint2D(),
            static_cast<int>(displayNbLandmarks * kReprojectionVertices),
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
        auto root = new QSGGeometryNode;
        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;
        {
          geometryPoint = new QSGGeometry(
            QSGGeometry::defaultAttributes_ColoredPoint2D(),
            static_cast<int>(displayNbLandmarks),
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
      if (oldNode->childCount() < 7)
        return;
      auto* rootLine = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(5));
      auto* rootPoint = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(6));
      if (!rootLine || !rootPoint)
        return;

      rootLine->markDirty(QSGNode::DirtyGeometry);
      rootPoint->markDirty(QSGNode::DirtyGeometry);

      geometryLine = rootLine->geometry();
      geometryLine->allocate(
        static_cast<int>(displayNbLandmarks * kReprojectionVertices),
        static_cast<int>(0)
      );
      geometryPoint = rootPoint->geometry();
      geometryPoint->allocate(
        static_cast<int>(displayNbLandmarks),
        static_cast<int>(0)
      );
    }

    if (displayNbLandmarks == 0)
      return;

    const auto& features = currentViewFeatures->features;

    if (features.empty())
      return; // nothing to do, no features in the current view

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
        point.x(), point.y(),
        color.red(), color.green(), color.blue(), color.alpha()
      );
    };
    const auto setVerticePoint = [&](unsigned int index, const QPointF& point, const QColor& color)
    {
      verticesPoints[index].set(
        point.x(), point.y(),
        color.red(), color.green(), color.blue(), color.alpha()
      );
    };

    const QColor reprojectionColor = _landmarkColor.darker(150);

    // Draw lines between reprojected points and features extracted
    int obsI = 0;
    for (int i = 0; i < features.size(); ++i)
    {
      const auto& f = features.at(i);
      float x = f->x();
      float y = f->y();
      float rx = f->rx();
      float ry = f->ry();

      bool isReconstructed = f->landmarkId() >= 0;
      if (isReconstructed)
      {
        unsigned int vidx = obsI * kReprojectionVertices;

        setVerticeLine(vidx, QPointF(x, y), reprojectionColor);
        setVerticeLine(vidx + 1, QPointF(rx, ry), reprojectionColor);

        setVerticePoint(obsI, QPointF(rx, ry), _landmarkColor);

        ++obsI;
      }
    }
  }

  QSGNode* FeaturesViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
  {
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

    updatePaintFeatures(oldNode, node);
    updatePaintTracks(oldNode, node);
    updatePaintMatches(oldNode, node);
    updatePaintLandmarks(oldNode, node);

    return node;
  }

} // namespace qtAliceVision
