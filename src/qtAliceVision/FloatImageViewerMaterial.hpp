#pragma once

#include "FloatTexture.hpp"

#include <QSGMaterial>
#include <QSGRendererInterface>
#include <QSGTexture>
#include <QVector2D>
#include <QVector4D>

#include <memory>

namespace qtAliceVision {


class FloatImageViewerMaterialShader;

/**
 * @brief Custom QSGMaterial carrying the per-frame uniform data and the floating-point image texture
 *        used by FloatImageViewerMaterialShader.
 *
 * @warning The Uniforms struct layout must stay in sync with the uniform block declared in
 *          FloatImageViewer.vert and FloatImageViewer.frag.
 */
class FloatImageViewerMaterial : public QSGMaterial
{
  public:
    FloatImageViewerMaterial();

    QSGMaterialType* type() const override;
    int compare(const QSGMaterial* other) const override;
    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode renderMode) const override;

    /**
     * @brief Uniform block mirrored on the CPU side.
     * @warning Layout and padding must exactly match FloatImageViewer.vert / .frag.
     */
    struct Uniforms
    {
        QVector4D channelOrder = QVector4D(0, 1, 2, 3); ///< Swizzle indices for channel display mode.
        QVector2D fisheyeCircleCoord = QVector2D(0, 0); ///< Normalised centre of the fisheye circle (UV space).
        float gamma = 1.f;                               ///< Gamma correction factor.
        float gain = 0.f;                                ///< Exposure gain.
        float fisheyeCircleRadius = 0.f;                 ///< Fisheye circle radius in UV space (0 = disabled).
        float aspectRatio = 0.f;                         ///< Aspect ratio used for fisheye circle correction.
    } uniforms;

    static_assert(sizeof(Uniforms) == 40, "Uniforms size mismatch — check alignment against FloatImageViewer shader UBO");

    bool dirtyUniforms = false;       ///< True when uniforms have been modified since the last GPU upload.
    bool appliedHoveringGamma = false; ///< True while the hover gamma boost is currently applied.

    std::unique_ptr<FloatTexture> texture; ///< The floating-point texture uploaded to the GPU.
};

}  // namespace qtAliceVision
