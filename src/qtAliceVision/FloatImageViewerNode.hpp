#pragma once

#include "FloatImageViewerMaterial.hpp"
#include "FloatTexture.hpp"

#include <QColor>
#include <QRectF>
#include <QSize>
#include <QSGGeometryNode>
#include <QVector2D>
#include <QVector4D>

#include <memory>

namespace qtAliceVision {

class Surface;

/**
 * @brief Scene graph node responsible for rendering a FloatImageViewer frame.
 *
 * Owns both the image geometry node (textured quad / distortion mesh) and a child
 * grid node used when the distortion grid overlay is enabled.
 * All setters mark the node dirty so Qt Quick re-renders on the next frame.
 */
class FloatImageViewerNode : public QSGGeometryNode
{
  public:
    /**
     * @brief Constructs the node with pre-allocated geometry buffers.
     * @param vertexCount Initial number of vertices for the image mesh.
     * @param indexCount  Initial number of indices for the image mesh (also used as the grid vertex count).
     */
    FloatImageViewerNode(int vertexCount, int indexCount);

    /**
     * @brief Reallocates geometry buffers when the mesh subdivision changes.
     * @param vertexCount New vertex count for the image mesh.
     * @param indexCount  New index count for the image mesh / vertex count for the grid.
     */
    void setSubdivisions(int vertexCount, int indexCount);

    /**
     * @brief Updates the mesh geometry, applies hover gamma, and redraws the grid overlay.
     * @param surface        Surface object providing vertex data and display settings.
     * @param textureSize    Size of the texture currently bound to this node.
     * @param downscaleLevel Downscale level applied to the loaded image.
     * @param canBeHovered   When true, a gamma boost is applied on mouse-over.
     * @param mouseJustLeft  True on the frame the cursor leaves the item (used to revert the gamma boost).
     */
    void updatePaintSurface(Surface& surface, QSize textureSize, int downscaleLevel, bool canBeHovered, bool mouseJustLeft);

    /**
     * @brief Updates the textured-rect geometry to fill the given bounding rectangle.
     * @param bounds Target rectangle in scene coordinates.
     */
    void setRect(const QRectF& bounds);

    /**
     * @brief Sets the channel swizzle order uploaded to the shader.
     * @param channelOrder Four-component index vector selecting r/g/b/a source channels.
     */
    void setChannelOrder(QVector4D channelOrder);

    /**
     * @brief Enables or disables alpha blending on the material.
     * @param value True to enable blending (required for RGBA mode).
     */
    void setBlending(bool value);

    /**
     * @brief Sets the gamma correction factor uploaded to the shader.
     * @param gamma Gamma value (1.0 = linear).
     */
    void setGamma(float gamma);

    /**
     * @brief Sets the exposure gain factor uploaded to the shader.
     * @param gain Gain multiplier (0.0 = no change in the shader's convention).
     */
    void setGain(float gain);

    /**
     * @brief Replaces the floating-point texture displayed by this node.
     * @param texture New texture to bind; ownership is transferred to the material.
     */
    void setTexture(std::unique_ptr<FloatTexture> texture);

    /**
     * @brief Sets the colour of the distortion grid overlay.
     * @param gridColor Desired grid line colour.
     */
    void setGridColor(const QColor& gridColor);

    /**
     * @brief Configures the fisheye circle crop parameters in the shader.
     * @param aspectRatio          Width-to-height (or height-to-width) ratio of the full-res image.
     * @param fisheyeCircleRadius  Radius of the fisheye circle in UV space [0, 0.5].
     * @param fisheyeCircleCoord   Normalised centre of the fisheye circle in UV space.
     */
    void setFisheye(float aspectRatio, float fisheyeCircleRadius, QVector2D fisheyeCircleCoord);

    /**
     * @brief Disables fisheye circle cropping (sets radius to 0 in the shader).
     */
    void resetFisheye();

  private:
    /**
     * @brief Returns the typed image material, asserting it is non-null.
     * @return Pointer to the FloatImageViewerMaterial owned by this node.
     */
    FloatImageViewerMaterial* mat() const;

    QSGGeometryNode* _gridNode = nullptr; ///< Child node rendering the distortion grid overlay.
};

}  // namespace qtAliceVision
