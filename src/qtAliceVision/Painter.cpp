#include <Painter.hpp>

#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QtDebug>

#if QT_CONFIG(opengl)
    #include <qopenglshaderprogram.h>
#else
    #error "opengl must be valid"
#endif

namespace qtAliceVision {

class PointMaterialShader;

class PointMaterial : public QSGMaterial
{
public:
    PointMaterial() : 
        _color(1, 1, 1, 1),
        _size(1.0f)
    {
    }

    void setColor(const QColor& color)
    {
        _color = color;
    }

    QColor getColor() const
    {
        return _color;
    }

    void setSize(const float & size)
    { 
        _size = size; 
    }

    float getSize() const 
    {
        return _size; 
    }

    QSGMaterialType* type() const override
    {
        static QSGMaterialType type;
        return &type;
    }

    int compare(const QSGMaterial* other) const override
    {
        Q_ASSERT(other && type() == other->type());
        return other == this ? 0 : (other > this ? 1 : -1);
    }

    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;

private:
    QColor _color;
    float _size;
};

class PointMaterialShader : public QSGMaterialShader
{
public:
    PointMaterialShader()
    {
        setShaderFileName(VertexStage, QLatin1String(":/shaders/FeaturesViewer.vert.qsb"));
        setShaderFileName(FragmentStage, QLatin1String(":/shaders/FeaturesViewer.frag.qsb"));
    }

    bool updateUniformData(RenderState& state, QSGMaterial* newMaterial, QSGMaterial* oldMaterial) override
    {
        bool changed = false;
        QByteArray* buf = state.uniformData();
        
        if (state.isMatrixDirty())
        {
            const QMatrix4x4 m = state.combinedMatrix();
            memcpy(buf->data() + 0, m.constData(), 64);
            changed = true;
        }

        auto* currentMaterial = static_cast<PointMaterial*>(newMaterial);
        
        if (currentMaterial != nullptr)
        {
            const QColor& color = currentMaterial->getColor();
            const float& size = currentMaterial->getSize();
            bool hasColorChanged = true;
            bool hasSizeChanged = true;
            
            if (oldMaterial != nullptr)
            {
                auto* previousMaterial = static_cast<PointMaterial*>(oldMaterial);
                QColor pc = previousMaterial->getColor();
                if (color == pc)
                {
                    hasColorChanged = false;
                }

                if (previousMaterial->getSize() == size)
                {
                    hasSizeChanged = false;
                }
            }

            if (state.isOpacityDirty() || hasColorChanged)
            {
                float opacity = state.opacity() * color.alphaF();
                QVector4D v(color.redF() * opacity, color.greenF() * opacity, color.blueF() * opacity, opacity);

                memcpy(buf->data() + 64, &v, 4 * 4);
                changed = true;
            }

            if (hasSizeChanged)
            {
                memcpy(buf->data() + 80, &size, 4);
                changed = true;
            }
        }

        return changed;
    }
};

QSGMaterialShader* PointMaterial::createShader(QSGRendererInterface::RenderMode) const  
{ 
    return new PointMaterialShader; 
}

Painter::Painter(const std::vector<std::string>& layers)
  : _layers(layers)
{}

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

        auto material = new PointMaterial;

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

void Painter::drawPoints(QSGNode* node, const std::string& layer, const std::vector<QPointF>& points, const QColor& color, float pointSize) const
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
    //geometry->setLineWidth(pointSize);

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

    auto* material = static_cast<PointMaterial*>(root->material());
    if (!material)
    {
        qDebug() << "[qtAliceVision] Painter::drawPoints: invalid material for layer " << layer;
        return;
    }

    material->setColor(color);
    material->setSize(pointSize);
}

void Painter::drawLines(QSGNode* node, const std::string& layer, const std::vector<QPointF>& points, const QColor& color, float lineWidth) const
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

    auto* material = static_cast<PointMaterial*>(root->material());
    if (!material)
    {
        qDebug() << "[qtAliceVision] Painter::drawLines: invalid material for layer " << layer;
        return;
    }

    material->setColor(color);
}

void Painter::drawTriangles(QSGNode* node, const std::string& layer, const std::vector<QPointF>& points, const QColor& color) const
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

    auto* material = static_cast<PointMaterial*>(root->material());
    if (!material)
    {
        qDebug() << "[qtAliceVision] Painter::drawTriangles: invalid material for layer " << layer;
        return;
    }

    material->setColor(color);
}

}  // namespace qtAliceVision
