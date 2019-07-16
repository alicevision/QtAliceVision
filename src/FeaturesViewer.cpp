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


namespace qtAliceVision 
{

void FeatureIORunnable::run()
{
    using namespace aliceVision;

    // unpack parameters
    QUrl folder;
    aliceVision::IndexT viewId;
    QString descType;
    std::tie(folder, viewId, descType) = _params;

    std::vector<feature::SIOPointFeature> features;    
    try 
    {
        std::unique_ptr<feature::ImageDescriber> describer = feature::createImageDescriber(feature::EImageDescriberType_stringToEnum(descType.toStdString())); 
        features = feature::getSIOPointFeatures(*sfm::loadFeatures({folder.toLocalFile().toStdString()}, viewId, *describer));
    }
    catch(std::exception& e)
    {
        qDebug() << "[QtAliceVision] Failed to load features (" << descType << ") for view: " << viewId
                 << "\n" << e.what();
    }
    QList<Feature*> feats;
    feats.reserve(static_cast<int>(features.size()));
    for(const auto f : features)
        feats.append(new Feature(f));
    Q_EMIT resultReady(feats);
}


FeaturesViewer::FeaturesViewer(QQuickItem* parent):
QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);

    // trigger repaint events
    connect(this, &FeaturesViewer::displayModeChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::colorChanged, this, &FeaturesViewer::update);
    connect(this, &FeaturesViewer::featuresChanged, this, &FeaturesViewer::update);

    // trigger features reload events
    connect(this, &FeaturesViewer::folderChanged, this, &FeaturesViewer::reload);
    connect(this, &FeaturesViewer::viewIdChanged, this, &FeaturesViewer::reload);
    connect(this, &FeaturesViewer::describerTypeChanged, this, &FeaturesViewer::reload);
}

FeaturesViewer::~FeaturesViewer()
{
    qDeleteAll(_features);
}


void FeaturesViewer::reload()
{
    qDeleteAll(_features);
    _features.clear();
    if(_clearBeforeLoad)
        Q_EMIT featuresChanged();
    _outdated = false;

    if(!_folder.isValid() || _viewId == aliceVision::UndefinedIndexT)
    {
        Q_EMIT featuresChanged();
        return;
    }

    if(!_loading)
    {
        setLoading(true);
        // load features from file in a seperate thread
        auto* ioRunnable = new FeatureIORunnable(FeatureIORunnable::IOParams(_folder, _viewId, _describerType));
        connect(ioRunnable, &FeatureIORunnable::resultReady, this, &FeaturesViewer::onResultReady);
        QThreadPool::globalInstance()->start(ioRunnable);
    }
    else
    {
        // mark current request as outdated
        _outdated = true;
    }
}

void FeaturesViewer::onResultReady(QList<Feature*> features)
{
    setLoading(false);

    // another request has been made while io thread was working
    if(_outdated)
    {
        // clear result and reload features from file with current parameters
        qDeleteAll(features);
        reload();
        return;
    }

    // update features
    _features = features;
    Q_EMIT featuresChanged();
}

QSGNode* FeaturesViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
{
    // Implementation remarks
    // Only one QSGGeometryNode containing all geometry needed to draw all features
    // is created. This is ATM the only solution that scales well and provides
    // good performance even for +100K feature points.
    // The number of created vertices varies depending on the selected display mode.

    QSGNode* node = nullptr;
    QSGGeometry* geometry = nullptr;
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

    if(!oldNode)
    {
        node = new QSGNode;
        // use VertexColorMaterial to later be able to draw selection in another color
        QSGVertexColorMaterial* material = new QSGVertexColorMaterial; 

        QSGGeometryNode* root = new QSGGeometryNode;
        {
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
        node = oldNode;
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

    // utility lambda to register a vertice
    const auto setVertice = [&](unsigned int index, const QPointF& point)
    {
        vertices[index].set(
            point.x(), point.y(), 
            _color.red(), _color.green(), _color.blue(), _color.alpha()
        );
    };

    for(int i = 0; i < _features.size(); ++i)
    {
        auto& feat = _features[i]->pointFeature();
        const auto radius = 0.5f*feat.scale();
        unsigned int vidx = i*featVertices;
        unsigned int iidx = i*featIndices;

        if(_displayMode == FeaturesViewer::Points)
        {
            setVertice(vidx, QPointF(feat.x(), feat.y()));
        }
        else 
        {
            QRectF rect(feat.x(), feat.y(), feat.scale(), feat.scale());
            rect.translate(-radius, -radius);
            QPointF tl = rect.topLeft();
            QPointF tr = rect.topRight();
            QPointF br = rect.bottomRight();
            QPointF bl = rect.bottomLeft();

            if(_displayMode == FeaturesViewer::Squares)
            {
                // create 2 triangles
                setVertice(vidx, tl);
                setVertice(vidx+1, tr);
                setVertice(vidx+2, br);
                setVertice(vidx+3, bl);
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
                    setVertice(vidx+lidx, points[k]);
                    setVertice(vidx+lidx+1, points[k+1]);
                }
                // orientation line: up vector (0, 1)
                const auto nbPoints = static_cast<unsigned int>(points.size());
                setVertice(vidx+nbPoints*2-2, rect.center());
                auto o2 = t.map(rect.center() - QPointF(0.0f, radius)); // rotate end point
                setVertice(vidx+nbPoints*2-1, o2);
            }
        }
    }
   return node;
}

}
