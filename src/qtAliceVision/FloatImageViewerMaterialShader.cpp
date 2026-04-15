#include "FloatImageViewerMaterialShader.hpp"
#include "FloatImageViewerMaterial.hpp"

#include <QMatrix4x4>

#include <cstring>

namespace qtAliceVision {

namespace {

/**
 * @brief CPU-side mirror of the uniform buffer object shared by FloatImageViewer.vert/.frag.
 *
 * The layout follows std140 rules:
 *  - mat4  mvp      → 64 bytes at offset 0
 *  - float opacity  →  4 bytes at offset 64
 *  - float _pad[3]  → 12 bytes of std140 padding to align the next vec4
 *  - Uniforms block → 40 bytes at offset 80
 *
 * @warning This struct must remain in sync with the GLSL uniform block in
 *          FloatImageViewer.vert and FloatImageViewer.frag.
 */
struct UniformBufferLayout
{
    float mvp[16];                              ///< Combined model-view-projection matrix.
    float opacity;                              ///< Item opacity in [0, 1].
    float _pad[3];                              ///< std140 padding — do not use.
    FloatImageViewerMaterial::Uniforms custom;  ///< Per-material shader parameters.
};

static_assert(offsetof(UniformBufferLayout, mvp)     ==  0, "UBO layout mismatch: mvp");
static_assert(offsetof(UniformBufferLayout, opacity) == 64, "UBO layout mismatch: opacity");
static_assert(offsetof(UniformBufferLayout, custom)  == 80, "UBO layout mismatch: custom uniforms");
static_assert(sizeof(UniformBufferLayout)            == 120, "UBO size mismatch");

}  // namespace

FloatImageViewerMaterialShader::FloatImageViewerMaterialShader()
{
    setShaderFileName(VertexStage, QLatin1String(":/shaders/FloatImageViewer.vert.qsb"));
    setShaderFileName(FragmentStage, QLatin1String(":/shaders/FloatImageViewer.frag.qsb"));
}

bool FloatImageViewerMaterialShader::updateUniformData(RenderState& state, QSGMaterial* newMaterial, QSGMaterial* oldMaterial)
{
    bool changed = false;
    QByteArray* buf = state.uniformData();
    Q_ASSERT(buf->size() >= static_cast<int>(sizeof(UniformBufferLayout)));

    if (state.isMatrixDirty())
    {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data() + offsetof(UniformBufferLayout, mvp), m.constData(), sizeof(UniformBufferLayout::mvp));
        changed = true;
    }

    if (state.isOpacityDirty())
    {
        const float opacity = state.opacity();
        memcpy(buf->data() + offsetof(UniformBufferLayout, opacity), &opacity, sizeof(UniformBufferLayout::opacity));
        changed = true;
    }

    auto* customMaterial = static_cast<FloatImageViewerMaterial*>(newMaterial);
    if (oldMaterial != newMaterial || customMaterial->dirtyUniforms)
    {
        memcpy(buf->data() + offsetof(UniformBufferLayout, custom), &customMaterial->uniforms, sizeof(UniformBufferLayout::custom));
        customMaterial->dirtyUniforms = false;
        changed = true;
    }

    return changed;
}

void FloatImageViewerMaterialShader::updateSampledImage(RenderState& state, int binding, QSGTexture** texture,
                                                        QSGMaterial* newMaterial, QSGMaterial* /*oldMaterial*/)
{
    auto* mat = static_cast<FloatImageViewerMaterial*>(newMaterial);
    if (binding == 1)
    {
        if (mat->texture)
        {
            mat->texture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
        }

        *texture = mat->texture.get();
    }
}

}  // namespace qtAliceVision
