#include "FeaturesViewer.hpp"

#include <QSGGeometryNode>
#include <QSGGeometry>
#include <QSGVertexColorMaterial>
#include <QSGFlatColorMaterial>

#include <QtMath>
#include <QThreadPool>
#include <QTransform>


namespace qtAliceVision 
{

FeaturesViewer::FeaturesViewer(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);

    // trigger repaint events
    connect(this, &FeaturesViewer::displayModeChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::colorChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::landmarkColorChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::descFeaturesChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::displayFeaturesChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::displayLandmarksChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::displayTracksChanged, this, &FeaturesViewer::update);
}

void FeaturesViewer::setMDescFeatures(MDescFeatures* descFeatures)
{
    if (_mdescFeatures == descFeatures)
        return;
    if (_mdescFeatures != nullptr)
    {
        disconnect(_mdescFeatures, &MDescFeatures::featuresReadyToDisplayChanged, this, &FeaturesViewer::descFeaturesChanged);
        disconnect(_mdescFeatures, &MDescFeatures::updateDisplayTracks, this, &FeaturesViewer::setDisplayTracks);
    }
    _mdescFeatures = descFeatures;
    if (_mdescFeatures != nullptr)
    {
        connect(_mdescFeatures, &MDescFeatures::featuresReadyToDisplayChanged, this, &FeaturesViewer::descFeaturesChanged);
        connect(_mdescFeatures, &MDescFeatures::updateDisplayTracks, this, &FeaturesViewer::setDisplayTracks);
    }
    Q_EMIT descFeaturesChanged();
}

void FeaturesViewer::updatePaintFeatures(QSGNode* oldNode, QSGNode* node)
{
    unsigned int kFeatVertices = 0;
    unsigned int kFeatIndices = 0;

    switch (_displayMode)
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
    std::size_t displayNbFeatures = _displayFeatures ? _mdescFeatures->getFeaturesData().size() : 0;
    QSGGeometry* geometry = nullptr;
    if (!oldNode)
    {
        if (_displayFeatures)
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
    }
    else
    {
        auto* root = static_cast<QSGGeometryNode*>(oldNode->firstChild());
        root->markDirty(QSGNode::DirtyGeometry);
        geometry = root->geometry();
        geometry->allocate(
            static_cast<int>(displayNbFeatures * kFeatVertices),
            static_cast<int>(displayNbFeatures * kFeatIndices)
        );
    }

    if (!_displayFeatures)
    {
        return;
    }

    switch (_displayMode)
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
        QColor c = _color; // (isReconstructed ? _colorReproj : _color);
        vertices[index].set(
            point.x(), point.y(),
            c.red(), c.green(), c.blue(), c.alpha()
        );
    };

    for (int i = 0; i < _mdescFeatures->getFeaturesData().size(); ++i)
    {
        const auto& f = _mdescFeatures->getFeaturesData().at(i);
        bool isReconstructed = f->landmarkId() > 0;
        auto& feat = f->pointFeature();
        const auto radius = feat.scale();
        const auto diag = 2.0 * feat.scale();
        unsigned int vidx = i * kFeatVertices;
        unsigned int iidx = i * kFeatIndices;

        if (_displayMode == FeaturesViewer::Points)
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

            if (_displayMode == FeaturesViewer::Squares)
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
            else if (_displayMode == FeaturesViewer::OrientedSquares)
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
    unsigned int kTracksVertices = 1;

    const std::size_t displayNbTracks = _displayTracks ? getNbTracks() - getNbLandmarks() : 0;

    QSGGeometry* geometryPoint = nullptr;
    if (!oldNode)
    {
        if (_displayTracks)
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

    }
    else
    {
        auto* rootPoint = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(1));
        rootPoint->markDirty(QSGNode::DirtyGeometry);
        geometryPoint = rootPoint->geometry();
        geometryPoint->allocate(
            static_cast<int>(displayNbTracks * kTracksVertices),
            static_cast<int>(0)
        );
    }

    if (!_displayTracks)
    {
        return;
    }

    geometryPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryPoint->setLineWidth(6.0f);

    QSGGeometry::ColoredPoint2D* verticesPoints = geometryPoint->vertexDataAsColoredPoint2D();

    const auto setVerticePoint = [&](unsigned int index, const QPointF& point)
    {
        QColor c = QColor(255, 127, 0);
        verticesPoints[index].set(
            point.x(), point.y(),
            c.red(), c.green(), c.blue(), c.alpha()
        );
    };

    // Draw points in the center of non validated tracks
    int obsI = 0;
    for (int i = 0; i < _mdescFeatures->getFeaturesData().size(); ++i)
    {
        const auto& f = _mdescFeatures->getFeaturesData().at(i);

        if (f->trackId() < 0)
            continue;
        if (getNbLandmarks() > 0 && f->landmarkId() >= 0)
            continue;

        if (obsI >= displayNbTracks)
        {
            qWarning() << "[QtAliceVision] updatePaintTracks ERROR on number of tracks";
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
    const unsigned int kReprojectionVertices = 2;
    //
    const int displayNbLandmarks = _displayLandmarks ? getNbLandmarks() : 0;

    QSGGeometry* geometryLine = nullptr;
    QSGGeometry* geometryPoint = nullptr;
    if (!oldNode)
    {
        if (_displayLandmarks)
        {
            {
                auto root = new QSGGeometryNode;
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
    }
    else
    {
        auto* rootLine = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(2));
        auto* rootPoint = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(3));

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

    if (!_displayLandmarks)
    {
        return;
    }

    geometryLine->setDrawingMode(QSGGeometry::DrawLines);
    geometryLine->setLineWidth(2.0f);

    geometryPoint->setDrawingMode(QSGGeometry::DrawPoints);
    geometryPoint->setLineWidth(6.0f);

    QSGGeometry::ColoredPoint2D* verticesLines = geometryLine->vertexDataAsColoredPoint2D();
    QSGGeometry::ColoredPoint2D* verticesPoints = geometryPoint->vertexDataAsColoredPoint2D();

    // utility lambda to register a vertex
    const auto setVerticeLine = [&](unsigned int index, const QPointF& point)
    {
        verticesLines[index].set(
            point.x(), point.y(),
            _landmarkColor.red(), _landmarkColor.green(), _landmarkColor.blue(), _landmarkColor.alpha()
        );
    };
    const auto setVerticePoint = [&](unsigned int index, const QPointF& point)
    {
        verticesPoints[index].set(
            point.x(), point.y(),
            _landmarkColor.red(), _landmarkColor.green(), _landmarkColor.blue(), _landmarkColor.alpha()
        );
    };

    // Draw lines between reprojected points and features extracted
    int obsI = 0;
    for (int i = 0; i < _mdescFeatures->getFeaturesData().size(); ++i)
    {
        const auto& f = _mdescFeatures->getFeaturesData().at(i);
        float x = f->x();
        float y = f->y();
        float rx = f->rx();
        float ry = f->ry();

        bool isReconstructed = f->landmarkId() >= 0;
        if(isReconstructed)
        {
            unsigned int vidx = obsI * kReprojectionVertices;

            setVerticeLine(vidx, QPointF(x, y));
            setVerticeLine(vidx+1, QPointF(rx, ry));

            setVerticePoint(obsI, QPointF(rx, ry));

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
    if(!oldNode)
    {
        node = new QSGNode;
    } 
    else
    {
        node = oldNode;
    }

    if (!_mdescFeatures)
        return node;

    updatePaintFeatures(oldNode, node);
    updatePaintTracks(oldNode, node);
    updatePaintLandmarks(oldNode, node);

    return node;
}

} // namespace
