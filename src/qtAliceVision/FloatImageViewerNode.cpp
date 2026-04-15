#include "FloatImageViewerNode.hpp"
#include "FloatImageViewerMaterial.hpp"
#include "Surface.hpp"

#include <QSGFlatColorMaterial>
#include <QSGGeometry>

namespace qtAliceVision {

FloatImageViewerNode::FloatImageViewerNode(int vertexCount, int indexCount)
{
    // --- Image mesh ---
    auto* m = new FloatImageViewerMaterial;
    setMaterial(m);
    setFlag(OwnsMaterial, true);

    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), vertexCount, indexCount);
    // Empty rect is intentional: setRect() will be called by updatePaintNode() before first render.
    QSGGeometry::updateTexturedRectGeometry(geometry, QRect(), QRect());
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);
    geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
    geometry->setVertexDataPattern(QSGGeometry::StaticPattern);
    setGeometry(geometry);
    setFlag(OwnsGeometry, true);

    // --- Grid overlay ---
    _gridNode = new QSGGeometryNode;

    // The grid vertex count equals the image index count.
    auto* gridGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), indexCount);
    gridGeometry->setDrawingMode(QSGGeometry::DrawLines);
    gridGeometry->setLineWidth(2);
    _gridNode->setGeometry(gridGeometry);
    _gridNode->setFlag(QSGNode::OwnsGeometry);  // use setFlag() — setFlags() replaces all flags

    auto* gridMaterial = new QSGFlatColorMaterial;
    _gridNode->setMaterial(gridMaterial);
    _gridNode->setFlag(QSGNode::OwnsMaterial);

    appendChildNode(_gridNode);
}

void FloatImageViewerNode::setSubdivisions(int vertexCount, int indexCount)
{
    geometry()->allocate(vertexCount, indexCount);
    markDirty(QSGNode::DirtyGeometry);

    // Grid vertex count equals the image index count.
    _gridNode->geometry()->allocate(indexCount);
    _gridNode->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
}

void FloatImageViewerNode::updatePaintSurface(Surface& surface, QSize textureSize, int downscaleLevel, bool canBeHovered, bool mouseJustLeft)
{
    // Apply / remove the hover gamma boost.
    if (canBeHovered)
    {
        auto* m = mat();
        if (surface.getMouseOver() && !m->appliedHoveringGamma)
        {
            setGamma(m->uniforms.gamma + 1.f);
            m->appliedHoveringGamma = true;
            markDirty(QSGNode::DirtyMaterial);
        }
        else if (mouseJustLeft && m->appliedHoveringGamma)
        {
            setGamma(m->uniforms.gamma - 1.f);
            m->appliedHoveringGamma = false;
            markDirty(QSGNode::DirtyMaterial);
        }
    }

    // Recompute mesh vertices when the surface topology has changed.
    if (surface.hasVerticesChanged())
    {
        auto* vertices = geometry()->vertexDataAsTexturedPoint2D();
        auto* indices  = geometry()->indexDataAsUShort();

        surface.update(vertices, indices, textureSize, downscaleLevel);

        geometry()->markIndexDataDirty();
        geometry()->markVertexDataDirty();
        markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);

        surface.fillVertices(vertices);
    }

    // Redraw or clear the grid overlay only when it is visible.
    if (surface.getDisplayGrid())
    {
        surface.computeGrid(_gridNode->geometry());
        _gridNode->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
    }
    else if (surface.hasVerticesChanged())
    {
        // Vertices changed while grid is hidden — clear residual data.
        surface.removeGrid(_gridNode->geometry());
        _gridNode->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
    }
}

void FloatImageViewerNode::setRect(const QRectF& bounds)
{
    QSGGeometry::updateTexturedRectGeometry(geometry(), bounds, QRectF(0, 0, 1, 1));
    markDirty(QSGNode::DirtyGeometry);
}

FloatImageViewerMaterial* FloatImageViewerNode::mat() const
{
    auto* m = static_cast<FloatImageViewerMaterial*>(material());
    Q_ASSERT(m);
    return m;
}

void FloatImageViewerNode::setChannelOrder(QVector4D channelOrder)
{
    auto* m = mat();
    m->uniforms.channelOrder = channelOrder;
    m->dirtyUniforms = true;
    markDirty(DirtyMaterial);
}

void FloatImageViewerNode::setBlending(bool value)
{
    mat()->setFlag(QSGMaterial::Blending, value);
    markDirty(DirtyMaterial);
}

void FloatImageViewerNode::setGamma(float gamma)
{
    auto* m = mat();
    m->uniforms.gamma = gamma;
    m->dirtyUniforms = true;
    markDirty(DirtyMaterial);
}

void FloatImageViewerNode::setGain(float gain)
{
    auto* m = mat();
    m->uniforms.gain = gain;
    m->dirtyUniforms = true;
    markDirty(DirtyMaterial);
}

void FloatImageViewerNode::setTexture(std::unique_ptr<FloatTexture> texture)
{
    auto* m = mat();
    m->texture = std::move(texture);
    markDirty(DirtyMaterial);
}

void FloatImageViewerNode::setGridColor(const QColor& gridColor)
{
    auto* m = static_cast<QSGFlatColorMaterial*>(_gridNode->material());
    m->setColor(gridColor);
    _gridNode->markDirty(QSGNode::DirtyMaterial);
}

void FloatImageViewerNode::setFisheye(float aspectRatio, float fisheyeCircleRadius, QVector2D fisheyeCircleCoord)
{
    auto* m = mat();
    m->uniforms.aspectRatio         = aspectRatio;
    m->uniforms.fisheyeCircleRadius = fisheyeCircleRadius;
    m->uniforms.fisheyeCircleCoord  = fisheyeCircleCoord;
    m->dirtyUniforms = true;
    markDirty(DirtyMaterial);
}

void FloatImageViewerNode::resetFisheye()
{
    auto* m = mat();
    m->uniforms.fisheyeCircleRadius = 0.f;
    m->dirtyUniforms = true;
    markDirty(DirtyMaterial);
}

}  // namespace qtAliceVision
