#include <Painter.hpp>

#include <QtDebug>
#include <QSGGeometry>
#include <QSGFlatColorMaterial>

namespace qtAliceVision {

Painter::Painter(const std::vector<std::string>& layers) : 
	_layers(layers)
{
}

std::size_t Painter::layerIndex(const std::string& layer) const
{
	for (std::size_t idx = 0; idx < _layers.size(); idx++)
	{
		if (_layers[idx] == layer) 
		{
			return idx;
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

	while (node->childCount() < _layers.size())
	{
		auto root = new QSGGeometryNode;
		
		auto geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
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

	std::size_t index = layerIndex(layer);
	if (index < 0) 
	{
		qDebug() << "[qtAliceVision] Painter::getGeometryNode: could not find corresponding index for layer " << layer;
		return nullptr;
	}

	auto* root = static_cast<QSGGeometryNode*>(node->childAtIndex(index));
	return root;
}

void Painter::clearLayer(QSGNode* node, 
						 const std::string& layer) const
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

void Painter::drawPoints(QSGNode* node, 
						 const std::string& layer, 
						 const std::vector<QPointF>& points, 
						 const QColor& color, 
						 float pointSize) const
{
	auto* root = getGeometryNode(node, layer);
	if (!root) 
	{
		qDebug() << "[qtAliceVision] Painter::drawPoints: failed to retrieve geometry node for layer " << layer;
		return;
	}

	root->markDirty(QSGNode::DirtyGeometry);
	auto geometry = root->geometry();
	geometry->allocate(points.size(), 0);

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
    	vertices[i].set(p.x(), p.y());
    }

    auto* material = static_cast<QSGFlatColorMaterial*>(root->material());
    if (!material) 
	{
		qDebug() << "[qtAliceVision] Painter::drawPoints: invalid material for layer " << layer;
		return;
	}

    material->setColor(color);
}

void Painter::drawLines(QSGNode* node, 
			   			const std::string& layer, 
			   			const std::vector<QLineF>& lines, 
			   			const QColor& color, 
			   			float lineWidth) const
{
	auto* root = getGeometryNode(node, layer);
	if (!root) 
	{
		qDebug() << "[qtAliceVision] Painter::drawLines: failed to retrieve geometry node for layer " << layer;
		return;
	}

	root->markDirty(QSGNode::DirtyGeometry);
	auto geometry = root->geometry();
	geometry->allocate(lines.size() * 2, 0);

	geometry->setDrawingMode(QSGGeometry::DrawLines);
    geometry->setLineWidth(lineWidth);

    auto* vertices = geometry->vertexDataAsPoint2D();
    if (!vertices) 
	{
		qDebug() << "[qtAliceVision] Painter::drawLines: invalid vertex data for layer " << layer;
		return;
	}

    for (std::size_t i = 0; i < lines.size(); i++)
    {
    	const QLineF& l = lines[i];
    	vertices[2 * i].set(l.x1(), l.y1());
    	vertices[2 * i + 1].set(l.x2(), l.y2());
    }

    auto* material = static_cast<QSGFlatColorMaterial*>(root->material());
    if (!material) 
	{
		qDebug() << "[qtAliceVision] Painter::drawLines: invalid material for layer " << layer;
		return;
	}

    material->setColor(color);
}

void Painter::drawTriangles(QSGNode* node,
							const std::string& layer,
							const std::vector<QPointF>& points,
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
	geometry->allocate(points.size(), 0);

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
    	vertices[i].set(p.x(), p.y());
    }

    auto* material = static_cast<QSGFlatColorMaterial*>(root->material());
    if (!material) 
	{
		qDebug() << "[qtAliceVision] Painter::drawTriangles: invalid material for layer " << layer;
		return;
	}

    material->setColor(color);
}

}
