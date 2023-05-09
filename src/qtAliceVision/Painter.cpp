#include <Painter.hpp>

#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QtDebug>

namespace qtAliceVision
{

Painter::Painter(const std::vector<std::string>& layers)
    : _layers(layers)
{
}

int Painter::layerIndex(const std::string& layer) const
{
    for (std::size_t idx = 0; idx < _layers.size(); idx++)
    {
        if (_layers[idx] == layer)
        {
            return static_cast<int>(idx);
        }
    }
    return -1;
}

bool Painter::ensureGeometry(QSGNode* node) const
{
    if (!node)
    {
        qDebug() << "[qtAliceVision] Painter::ensureGeometry: invalid node";
        return false;
    }

    while (node->childCount() < static_cast<int>(_layers.size()))
    {
        auto root = new QSGGeometryNode;

        auto geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0, 0, QSGGeometry::UnsignedIntType);
        geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
        geometry->setVertexDataPattern(QSGGeometry::StaticPattern);

        auto material = new QSGFlatColorMaterial;

        root->setGeometry(geometry);
        root->setMaterial(material);
        root->setFlags(QSGNode::OwnsGeometry);
        root->setFlags(QSGNode::OwnsMaterial);
        node->appendChildNode(root);
    }

    return true;
}

QSGGeometryNode* Painter::getGeometryNode(QSGNode* node, const std::string& layer) const
{
    if (!ensureGeometry(node))
    {
        qDebug() << "[qtAliceVision] Painter::getGeometryNode: could not ensure node layers are correct";
        return nullptr;
    }

    int index = layerIndex(layer);
    if (index < 0)
    {
        qDebug() << "[qtAliceVision] Painter::getGeometryNode: could not find corresponding index for layer " << layer;
        return nullptr;
    }

    auto* root = static_cast<QSGGeometryNode*>(node->childAtIndex(index));
    return root;
}

void Painter::clearLayer(QSGNode* node, const std::string& layer) const
{
    auto* root = getGeometryNode(node, layer);
    if (!root)
    {
        qDebug() << "[qtAliceVision] Painter::clearLayer: failed to retrieve geometry node for layer " << layer;
        return;
    }

    root->markDirty(QSGNode::DirtyGeometry);
    auto geometry = root->geometry();
    geometry->allocate(0, 0);
}

void Painter::drawPoints(QSGNode* node, const std::string& layer, const std::vector<QPointF>& points,
                         const QColor& color, float pointSize) const
{
    auto* root = getGeometryNode(node, layer);
    if (!root)
    {
        qDebug() << "[qtAliceVision] Painter::drawPoints: failed to retrieve geometry node for layer " << layer;
        return;
    }

    root->markDirty(QSGNode::DirtyGeometry);
    auto geometry = root->geometry();
    geometry->allocate(static_cast<int>(points.size()), 0);

    geometry->setDrawingMode(QSGGeometry::DrawPoints);
    geometry->setLineWidth(pointSize);

    auto* vertices = geometry->vertexDataAsPoint2D();
    if (!vertices)
    {
        qDebug() << "[qtAliceVision] Painter::drawPoints: invalid vertex data for layer " << layer;
        return;
    }

    for (std::size_t i = 0; i < points.size(); i++)
    {
        const QPointF& p = points[i];
        vertices[i].set(static_cast<float>(p.x()), static_cast<float>(p.y()));
    }

    auto* material = static_cast<QSGFlatColorMaterial*>(root->material());
    if (!material)
    {
        qDebug() << "[qtAliceVision] Painter::drawPoints: invalid material for layer " << layer;
        return;
    }

    material->setColor(color);
}

void Painter::drawLines(QSGNode* node, const std::string& layer, const std::vector<QPointF>& points,
                        const QColor& color, float lineWidth) const
{
    auto* root = getGeometryNode(node, layer);
    if (!root)
    {
        qDebug() << "[qtAliceVision] Painter::drawLines: failed to retrieve geometry node for layer " << layer;
        return;
    }

    root->markDirty(QSGNode::DirtyGeometry);
    auto geometry = root->geometry();
    geometry->allocate(static_cast<int>(points.size()), 0);

    geometry->setDrawingMode(QSGGeometry::DrawLines);
    geometry->setLineWidth(lineWidth);

    auto* vertices = geometry->vertexDataAsPoint2D();
    if (!vertices)
    {
        qDebug() << "[qtAliceVision] Painter::drawLines: invalid vertex data for layer " << layer;
        return;
    }

    for (std::size_t i = 0; i < points.size(); i++)
    {
        const QPointF& p = points[i];
        vertices[i].set(static_cast<float>(p.x()), static_cast<float>(p.y()));
    }

    auto* material = static_cast<QSGFlatColorMaterial*>(root->material());
    if (!material)
    {
        qDebug() << "[qtAliceVision] Painter::drawLines: invalid material for layer " << layer;
        return;
    }

    material->setColor(color);
}

void Painter::drawTriangles(QSGNode* node, const std::string& layer, const std::vector<QPointF>& points,
                            const QColor& color) const
{
    auto* root = getGeometryNode(node, layer);
    if (!root)
    {
        qDebug() << "[qtAliceVision] Painter::drawTriangles: failed to retrieve geometry node for layer " << layer;
        return;
    }

    root->markDirty(QSGNode::DirtyGeometry);
    auto geometry = root->geometry();
    geometry->allocate(static_cast<int>(points.size()), 0);

    geometry->setDrawingMode(QSGGeometry::DrawTriangles);

    auto* vertices = geometry->vertexDataAsPoint2D();
    if (!vertices)
    {
        qDebug() << "[qtAliceVision] Painter::drawTriangles: invalid vertex data for layer " << layer;
        return;
    }

    for (std::size_t i = 0; i < points.size(); i++)
    {
        const QPointF& p = points[i];
        vertices[i].set(static_cast<float>(p.x()), static_cast<float>(p.y()));
    }

    auto* material = static_cast<QSGFlatColorMaterial*>(root->material());
    if (!material)
    {
        qDebug() << "[qtAliceVision] Painter::drawTriangles: invalid material for layer " << layer;
        return;
    }

    material->setColor(color);
}

} // namespace qtAliceVision
