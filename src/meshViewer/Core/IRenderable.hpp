#pragma once

#include <rhi/qrhi.h>
#include "SceneState.hpp"

class LayerItem;
class SceneView;

class IRenderable
{
public:
    virtual ~IRenderable() = default;

    virtual void sync(LayerItem *layer, SceneView *view) { Q_UNUSED(layer) Q_UNUSED(view) }
    virtual void initialize(QRhi *rhi, QRhiRenderPassDescriptor *rpDesc) = 0;
    virtual void prepare(QRhiResourceUpdateBatch *batch, const SceneState &state) = 0;
    virtual void render(QRhiCommandBuffer *cb, const SceneState &state) = 0;
};
