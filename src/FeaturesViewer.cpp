#include "FeaturesViewer.hpp"

#include <QSGGeometryNode>
#include <QSGGeometry>
#include <QSGVertexColorMaterial>
#include <QSGFlatColorMaterial>

#include <QtMath>
#include <QThreadPool>
#include <QTransform>

#include <aliceVision/feature/PointFeature.hpp>
#include <aliceVision/feature/ImageDescriber.hpp>
#include <aliceVision/sfm/pipeline/regionsIO.hpp>

#include <aliceVision/sfmDataIO/sfmDataIO.hpp>


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
    connect(this, &FeaturesViewer::featuresChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::displayFeaturesChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::displayLandmarksChanged, this, &FeaturesViewer::update);

    // trigger features reload events
    connect(this, &FeaturesViewer::folderChanged, this, &FeaturesViewer::reloadFeatures);
    connect(this, &FeaturesViewer::viewIdChanged, this, &FeaturesViewer::reloadFeatures);
    connect(this, &FeaturesViewer::describerTypeChanged, this, &FeaturesViewer::reloadFeatures);

    connect(this, &FeaturesViewer::sfmDataChanged, this, &FeaturesViewer::updateFeatureFromSfM);
}

FeaturesViewer::~FeaturesViewer()
{
    qDeleteAll(_features);
}

void FeaturesViewer::reloadFeatures()
{
    qDeleteAll(_features);
    _features.clear();
    if(_clearFeaturesBeforeLoad)
        Q_EMIT featuresChanged();

    _outdatedFeatures = false;

    if(!_folder.isValid() || _viewId == aliceVision::UndefinedIndexT)
    {
        qWarning() << "[QtAliceVision] FeaturesViewer::reloadFeatures: No valid folder of view.";
        Q_EMIT featuresChanged();
        return;
    }

    if(!_loadingFeatures)
    {
        qWarning() << "[QtAliceVision] FeaturesViewer::reloadFeatures: Start loading features.";
        setLoadingFeatures(true);

        // load features from file in a seperate thread
        auto* ioRunnable = new FeatureIORunnable(FeatureIORunnable::IOParams(_folder, _viewId, _describerType));
        connect(ioRunnable, &FeatureIORunnable::resultReady, this, &FeaturesViewer::onFeaturesResultReady);
        QThreadPool::globalInstance()->start(ioRunnable);
    }
    else
    {
        qWarning() << "[QtAliceVision] FeaturesViewer::reloadFeatures: Mark as outdated.";
        // mark current request as outdated
        _outdatedFeatures = true;
    }
}

void FeaturesViewer::setMSfmData(MSfMData* sfmData)
{
    // qWarning() << "[QtAliceVision] FeaturesViewer::setMSfmData: sfmData: " << long(sfmData);
    if(_msfmData != nullptr)
    {
        disconnect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    _msfmData = sfmData;
    if(_msfmData != nullptr)
    {
        connect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    // qWarning() << "[QtAliceVision] FeaturesViewer::setMSfmData: _msfmData: " << long(_msfmData);
    Q_EMIT sfmDataChanged();
}

void FeaturesViewer::onFeaturesResultReady(QList<MFeature*> features)
{

    // another request has been made while io thread was working
    if(_outdatedFeatures)
    {
        // clear result and reload features from file with current parameters
        qDeleteAll(features);
        reloadFeatures();
        return;
    }

    // update features
    _features = features;
    setLoadingFeatures(false);
    updateFeatureFromSfM();
    Q_EMIT featuresChanged();
}

void FeaturesViewer::clearSfMFromFeatures()
{
    // qWarning() << "[QtAliceVision] clearSfMFromFeatures: start";
    for(const auto feature: _features)
    {
        feature->clearReconstructionInfo();
    }
    // qWarning() << "[QtAliceVision] clearSfMFromFeatures: end";
}

void FeaturesViewer::updateFeatureFromSfM()
{
    // qWarning() << "[QtAliceVision] updateFeatureFromSfM _msfmData: " << long(_msfmData);

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] updateFeatureFromSfM: no SfMData";
        clearSfMFromFeatures();
        return;
    }
    if(_msfmData->status() != MSfMData::Ready)
    {
        qWarning() << "[QtAliceVision] updateFeatureFromSfM: SfMData is not ready: " << _msfmData->status();
        clearSfMFromFeatures();
        return;
    }
    if(_msfmData->rawData().getViews().empty())
    {
        qWarning() << "[QtAliceVision] updateFeatureFromSfM: SfMData is empty";
        clearSfMFromFeatures();
        return;
    }
    if(!_msfmData->rawData().isPoseAndIntrinsicDefined(_viewId))
    {
        qWarning() << "[QtAliceVision] updateFeatureFromSfM: SfMData has no valid pose and intrinsic for view " << _viewId;
        clearSfMFromFeatures();
        return;
    }

    _nbLandmarks = 0;
    // Update newly loaded features with information from the sfmData
    aliceVision::feature::EImageDescriberType descType = aliceVision::feature::EImageDescriberType_stringToEnum(_describerType.toStdString());

    const auto& view = _msfmData->rawData().getView(_viewId);
    const aliceVision::sfmData::CameraPose pose = _msfmData->rawData().getPose(view);
    const aliceVision::geometry::Pose3 camTransform = pose.getTransform();
    const aliceVision::camera::IntrinsicBase* intrinsic = _msfmData->rawData().getIntrinsicPtr(view.getIntrinsicId());

   // qWarning() << "[QtAliceVision] updateFeatureFromSfM SfMData ready to compute: ";

    int numLandmark = 0;
    for(const auto& landmark: _msfmData->rawData().getLandmarks())
    {
        //qWarning() << "[QtAliceVision] updateFeatureFromSfM SfMData numLandmark: " << numLandmark;

        if(landmark.second.descType != descType)
            continue;

        auto itObs = landmark.second.observations.find(_viewId);
        if(itObs != landmark.second.observations.end())
        {          
            // setup landmark id and landmark 2d reprojection in the current view
            aliceVision::Vec2 r = intrinsic->project(camTransform, landmark.second.X);

            if (itObs->second.id_feat >= 0 && itObs->second.id_feat < _features.size())
            {
                _features.at(itObs->second.id_feat)->setReconstructionInfo(landmark.first, r.cast<float>());
            }
            else if(!_features.empty())
            {
                qWarning() << "[QtAliceVision] ---------- ERROR id_feat: " << itObs->second.id_feat << ", size: " << _features.size();
            }

            ++_nbLandmarks;
            //  qWarning() << "[QtAliceVision] updateFeatureFromSfM SfMData nbObservation: " << _nbObservations;
        }
        ++numLandmark;
    }

    Q_EMIT featuresChanged();
}

void FeaturesViewer::updatePaintFeatures(QSGNode* oldNode, QSGNode* node)
{
    unsigned int kFeatVertices = 0;
    unsigned int kFeatIndices = 0;

    switch(_displayMode)
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
    std::size_t displayNbFeatures = _displayFeatures ? _features.size() : 0;
    QSGGeometry* geometry = nullptr;
    if(!oldNode)
    {
        if(_displayFeatures)
        {
            auto root = new QSGGeometryNode;
            {
                // use VertexColorMaterial to later be able to draw selection in another color
                auto material = new QSGVertexColorMaterial;
                geometry = new QSGGeometry(
                    QSGGeometry::defaultAttributes_ColoredPoint2D(),
                    static_cast<int>(displayNbFeatures * kFeatVertices),
                    static_cast<int>(displayNbFeatures* kFeatIndices),
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

    if(!_displayFeatures)
    {
        // qWarning() << "[QtAliceVision] updatePaintLandmarks display features disabled";
        return;
    }

    switch(_displayMode)
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

    for(int i = 0; i < _features.size(); ++i)
    {
        const auto& f = _features.at(i);
        bool isReconstructed = f->landmarkId() > 0;
        auto& feat = f->pointFeature();
        const auto radius = feat.scale();
        const auto diag = 2.0 * feat.scale();
        unsigned int vidx = i*kFeatVertices;
        unsigned int iidx = i*kFeatIndices;

        if(_displayMode == FeaturesViewer::Points)
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

            if(_displayMode == FeaturesViewer::Squares)
            {
                // create 2 triangles
                setVertice(vidx, tl, isReconstructed);
                setVertice(vidx+1, tr, isReconstructed);
                setVertice(vidx+2, br, isReconstructed);
                setVertice(vidx+3, bl, isReconstructed);
                indices[iidx] = vidx;
                indices[iidx+1] = vidx+1;
                indices[iidx+2] = vidx+2;
                indices[iidx+3] = vidx+2;
                indices[iidx+4] = vidx+3;
                indices[iidx+5] = vidx;
            }
            else if(_displayMode == FeaturesViewer::OrientedSquares)
            {
                // compute angle: use feature angle and remove self rotation
                const auto radAngle = -feat.orientation()-qDegreesToRadians(rotation());
                // generate a QTransform that represent this rotation
                const auto t = QTransform().translate(feat.x(), feat.y()).rotateRadians(radAngle).translate(-feat.x(), -feat.y());

                // create lines, each vertice has to be duplicated (A->B, B->C, C->D, D->A) since we use GL_LINES
                std::vector<QPointF> points = {t.map(tl), t.map(tr), t.map(br), t.map(bl), t.map(tl)};
                for(unsigned int k = 0; k < points.size(); ++k)
                {
                    auto lidx = k*2; // local index
                    setVertice(vidx+lidx, points[k], isReconstructed);
                    setVertice(vidx+lidx+1, points[k+1], isReconstructed);
                }
                // orientation line: up vector (0, 1)
                const auto nbPoints = static_cast<unsigned int>(points.size());
                setVertice(vidx+nbPoints*2-2, rect.center(), isReconstructed);
                auto o2 = t.map(rect.center() - QPointF(0.0f, radius)); // rotate end point
                setVertice(vidx+nbPoints*2-1, o2, isReconstructed);
            }
        }
    }

}

void FeaturesViewer::updatePaintLandmarks(QSGNode* oldNode, QSGNode* node)
{
    const unsigned int kReprojectionVertices = 2;
    //
    int displayNbLandmarks = _displayLandmarks ? _nbLandmarks : 0;

    QSGGeometry* geometryLine = nullptr;
    QSGGeometry* geometryPoint = nullptr;
    if(!oldNode)
    {
        if(_displayLandmarks)
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
        auto* rootLine = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(1));
        auto* rootPoint = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(2));

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

    if(!_displayLandmarks)
    {
        // qWarning() << "[QtAliceVision] updatePaintLandmarks display landmarks disabled";
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
    for(int i = 0; i < _features.size(); ++i)
    {
        const auto& f = _features.at(i);
        float x = f->x();
        float y = f->y();
        float rx = f->rx();
        float ry = f->ry();

        bool isReconstructed = f->landmarkId() >= 0;
        if(isReconstructed)
        {
            auto& feat = f->pointFeature();

            unsigned int vidx = obsI * kReprojectionVertices;

            setVerticeLine(vidx, QPointF(x, y));
            setVerticeLine(vidx+1, QPointF(rx, ry));

            setVerticePoint(obsI, QPointF(rx, ry));

            ++obsI;
        }
    }

    qWarning() << "[QtAliceVision] updatePaintLandmarks _nbLandmarks: " << displayNbLandmarks << ", obsI: " << obsI;
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

    updatePaintFeatures(oldNode, node);

    updatePaintLandmarks(oldNode, node);

    return node;
}

}
