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
    connect(this, &FeaturesViewer::featuresChanged, this, &FeaturesViewer::update);

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
    if(_msfmData != nullptr)
    {
        disconnect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    _msfmData = sfmData;
    if(_msfmData != nullptr)
    {
        connect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
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
    for(const auto feature: _features)
    {
        feature->clearReconstructionInfo();
    }
}

void FeaturesViewer::updateFeatureFromSfM()
{
    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] updateFeatureFromSfM: no SfMData";
        clearSfMFromFeatures();
        return;
    }
    if(_msfmData->status() != MSfMData::Ready)
    {
        qWarning() << "[QtAliceVision] updateFeatureFromSfM: SfMData is not ready";
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

    _nbObservations = 0;
    // Update newly loaded features with information from the sfmData
    aliceVision::feature::EImageDescriberType descType = aliceVision::feature::EImageDescriberType_stringToEnum(_describerType.toStdString());

    const auto& view = _msfmData->rawData().getView(_viewId);
    const aliceVision::sfmData::CameraPose pose = _msfmData->rawData().getPose(view);
    const aliceVision::geometry::Pose3 camTransform = pose.getTransform();
    const aliceVision::camera::IntrinsicBase* intrinsic = _msfmData->rawData().getIntrinsicPtr(view.getIntrinsicId());

    for(const auto& landmark: _msfmData->rawData().getLandmarks())
    {
        if(landmark.second.descType != descType)
            continue;
        auto itObs = landmark.second.observations.find(_viewId);
        if(itObs != landmark.second.observations.end())
        {          
            // setup landmark id and landmark 2d reprojection in the current view
            aliceVision::Vec2 r = intrinsic->project(camTransform, landmark.second.X);
            _features.at(itObs->second.id_feat)->setReconstructionInfo(landmark.first, r.cast<float>());
            ++_nbObservations;
        }
    }

    Q_EMIT featuresChanged();
}

void FeaturesViewer::updatePaintFeatures(QSGNode* oldNode, QSGNode* node)
{
    unsigned int featVertices = 0;
    unsigned int featIndices = 0;

    switch(_displayMode)
    {
    case FeaturesViewer::Points:
        featVertices = 1;
        break;
    case FeaturesViewer::Squares:
        featVertices = 4;
        featIndices = 6;
        break;
    case FeaturesViewer::OrientedSquares:
        featVertices = (4 * 2) + 2;  // doubled rectangle points + orientation line
        break;
    }

    QSGGeometry* geometry = nullptr;
    if(!oldNode)
    {
        auto root = new QSGGeometryNode;
        {
            // use VertexColorMaterial to later be able to draw selection in another color
            auto material = new QSGVertexColorMaterial;
            geometry = new QSGGeometry(
                QSGGeometry::defaultAttributes_ColoredPoint2D(),
                static_cast<int>(_features.size() * featVertices),
                static_cast<int>(_features.size() * featIndices),
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
        auto* root = static_cast<QSGGeometryNode*>(oldNode->firstChild());
        root->markDirty(QSGNode::DirtyGeometry);
        geometry = root->geometry();
        geometry->allocate(
            static_cast<int>(_features.size() * featVertices),
            static_cast<int>(_features.size() * featIndices)
        );
    }

    switch(_displayMode)
    {
    case FeaturesViewer::Points:
        geometry->setDrawingMode(QSGGeometry::DrawPoints);
        geometry->setLineWidth(3.0f);
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
        QColor c = (isReconstructed ? _colorReproj : _color);
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
        const auto radius = feat.scale(); // TODO: size
        const auto diag = 2.0 * feat.scale();
        unsigned int vidx = i*featVertices;
        unsigned int iidx = i*featIndices;

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

void FeaturesViewer::updatePaintObservations(QSGNode* oldNode, QSGNode* node)
{
    unsigned int reprojectionVertices = 2;
    //

    QSGGeometry* geometry = nullptr;
    if(!oldNode)
    {
        auto root = new QSGGeometryNode;
        // use VertexColorMaterial to later be able to draw selection in another color
        auto material = new QSGVertexColorMaterial;
        {
            geometry = new QSGGeometry(
                QSGGeometry::defaultAttributes_ColoredPoint2D(),
                static_cast<int>(_nbObservations * reprojectionVertices),
                static_cast<int>(0),
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
        auto* root = static_cast<QSGGeometryNode*>(oldNode->lastChild());
        root->markDirty(QSGNode::DirtyGeometry);
        geometry = root->geometry();
        geometry->allocate(
            static_cast<int>(_nbObservations * reprojectionVertices),
            static_cast<int>(0)
        );
    }

    geometry->setDrawingMode(QSGGeometry::DrawLines);
    geometry->setLineWidth(1.0f);

    QSGGeometry::ColoredPoint2D* vertices = geometry->vertexDataAsColoredPoint2D();

    // utility lambda to register a vertex
    const auto setVertice = [&](unsigned int index, const QPointF& point, bool isReconstructed)
    {
        vertices[index].set(
            point.x(), point.y(),
            _colorReproj.red(), _colorReproj.green(), _colorReproj.blue(), _colorReproj.alpha()
        );
    };

    geometry->setDrawingMode(QSGGeometry::DrawLines);
    geometry->setLineWidth(1.0f);
    int obsI = 0;
    // Draw lines between reprojected points and features extracted
    for(int i = 0; i < _features.size(); ++i)
    {
        const auto& f = _features.at(i);
        float x = f->x();
        float y = f->y();
        float rx = f->rx();
        float ry = f->ry();

        bool isReconstructed = f->landmarkId() > 0;
        if(isReconstructed)
        {
            auto& feat = f->pointFeature();

            unsigned int vidx = obsI * reprojectionVertices;

            setVertice(vidx, QPointF(x, y), isReconstructed);
            setVertice(vidx+1, QPointF(rx, ry), isReconstructed);
            ++obsI;
        }
   }
   qWarning() << "[QtAliceVision] updatePaintObservations _nbObservations: " << _nbObservations << ", obsI: " << obsI;
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
    // updatePaintObservations(oldNode, node);

    return node;
}

}
