#pragma once

#include <rhi/qrhi.h>
#include "SceneState.hpp"

class LayerItem;
class SceneView;

/**
 * @brief Render-thread counterpart of a LayerItem.
 *
 * Created by LayerItem::createRenderable() and driven by SceneRenderer. The lifecycle is:
 *  - initialize(): create the GPU pipelines and resources (called once a QRhi is available).
 *  - sync(): copy the layer state while the GUI thread is blocked.
 *  - prepare(): upload per-frame data into the resource update batch.
 *  - render(): record draw calls inside the active render pass.
 */
class IRenderable
{
public:
    virtual ~IRenderable() = default;

    /**
     * @brief Copy the state of the owning layer; called while the GUI thread is blocked.
     * @param[in] layer Layer this renderable was created from.
     * @param[in] view Scene view hosting the layer.
     */
    virtual void sync(LayerItem *layer, SceneView *view)
    {
        Q_UNUSED(layer)
        Q_UNUSED(view)
    }
    /**
     * @brief Create the GPU resources and pipelines.
     * @param[in] rhi Rendering hardware interface.
     * @param[in] rpDesc Render pass descriptor the pipelines must be compatible with.
     */
    virtual void initialize(QRhi *rhi, QRhiRenderPassDescriptor *rpDesc) = 0;
    /**
     * @brief Queue the resource uploads needed for the next frame.
     * @param[in,out] batch Resource update batch submitted before the render pass.
     * @param[in] state Per-frame scene state.
     * @note Called even when the layer is hidden.
     */
    virtual void prepare(QRhiResourceUpdateBatch *batch, const SceneState &state) = 0;
    /**
     * @brief Record the draw calls inside the current render pass.
     * @param[in] cb Command buffer with an active render pass.
     * @param[in] state Per-frame scene state.
     */
    virtual void render(QRhiCommandBuffer *cb, const SceneState &state) = 0;
};
