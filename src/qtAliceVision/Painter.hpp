#pragma once

#include <QColor>
#include <QPointF>
#include <QSGGeometryNode>
#include <QSGNode>

#include <string>
#include <vector>

namespace qtAliceVision {

/**
 * @brief Utility class for abstracting the painting mechanisms in the Qt scene graph.
 *
 * The painting order is managed by a system of layers, identified by their names.
 * The first layer will be drawn behind all the others and the last layer will be drawn on top.
 *
 * Each layer is drawn using a flat color material, meaning all geometry will have the same color.
 */
class Painter
{
  public:
    /**
     * @brief Construct a Painter object with the given layers.
     * @param layers Layer names ordered from first to last drawn.
     */
    explicit Painter(const std::vector<std::string>& layers);

    /**
     * @brief Clear the content of a layer.
     * @param node Scene graph node in which the drawing occurs.
     * @param layer Layer name.
     */
    void clearLayer(QSGNode* node, const std::string& layer) const;

    /**
     * @brief Clear a layer and draw points on it.
     * @param node Scene graph node in which the drawing occurs.
     * @param layer Layer name.
     * @param points 2D points to draw.
     * @param color Points color (same for all points).
     * @param pointSize Points size (same for all points).
     */
    void drawPoints(QSGNode* node, const std::string& layer, const std::vector<QPointF>& points, const QColor& color, float pointSize) const;

    /**
     * @brief Clear a layer and draw lines on it.
     * @param node Scene graph node in which the drawing occurs.
     * @param layer Layer name.
     * @param points Line corners as 2D points.
     * @param color Lines color (same for all lines).
     * @param lineWidth Lines width (same for all lines).
     */
    void drawLines(QSGNode* node, const std::string& layer, const std::vector<QPointF>& points, const QColor& color, float lineWidth) const;

    /**
     * @brief Clear a layer and draw triangles on it.
     * @param node Scene graph node in which the drawing occurs.
     * @param layer Layer name.
     * @param points Triangle corners as 2D points.
     * @param color Triangles color (same for all triangles).
     */
    void drawTriangles(QSGNode* node, const std::string& layer, const std::vector<QPointF>& points, const QColor& color) const;

  private:
    /// Layers stack, ordered from first to last drawn.
    std::vector<std::string> _layers;

    /**
     * @brief Retrieve the index of a layer in the layers stack.
     * @param layer Layer name.
     * @return Layer index.
     */
    int layerIndex(const std::string& layer) const;

    /**
     * @brief Add children nodes if necessary to the given scene graph node to match this object's layers.
     * @param node Scene graph node in which the drawing occurs.
     * @return Whether of not a problem occured.
     */
    bool ensureGeometry(QSGNode* node) const;

    /**
     * @brief Retrieve the geometry node corresponding to a given layer.
     * @param node Scene graph node in which the drawing occurs.
     * @param layer Layer name.
     * @return Corresponding geometry node if it exists, otherwise nullptr.
     */
    QSGGeometryNode* getGeometryNode(QSGNode* node, const std::string& layer) const;
};

}  // namespace qtAliceVision
