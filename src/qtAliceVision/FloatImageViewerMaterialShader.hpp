#pragma once

#include <QSGMaterialShader>
#include <QSGTexture>

namespace qtAliceVision {

/**
 * @brief QSGMaterialShader that drives the FloatImageViewer render pipeline.
 *
 * Loads the compiled FloatImageViewer vertex and fragment shaders, uploads the
 * per-frame uniform block (matrix, opacity, gamma/gain/channel/fisheye parameters)
 * and binds the FloatTexture to sampler binding 1.
 */
class FloatImageViewerMaterialShader : public QSGMaterialShader
{
  public:
    FloatImageViewerMaterialShader();

    /**
     * @brief Uploads the combined MVP matrix, opacity, and custom uniforms to the GPU buffer.
     * @param state   Current render state providing the matrix and opacity.
     * @param newMaterial  Incoming material carrying the dirty uniform data.
     * @param oldMaterial  Previous material (used to detect material changes).
     * @return True if any data was written to the uniform buffer.
     */
    bool updateUniformData(RenderState& state, QSGMaterial* newMaterial, QSGMaterial* oldMaterial) override;

    /**
     * @brief Commits the FloatTexture for sampler binding 1 and sets the output texture pointer.
     * @param state       Current render state providing the RHI and resource update batch.
     * @param binding     Sampler binding index being updated.
     * @param texture     Output — set to the material's FloatTexture when binding == 1.
     * @param newMaterial Incoming material carrying the texture.
     * @param oldMaterial Previous material (unused).
     */
    void updateSampledImage(RenderState& state, int binding, QSGTexture** texture,
                            QSGMaterial* newMaterial, QSGMaterial* oldMaterial) override;
};

}  // namespace qtAliceVision
