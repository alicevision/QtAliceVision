#pragma once

#include <Core/IRenderable.hpp>
#include <SfmDataLayer/SfmCameraRenderable.hpp>
#include <SfmDataLayer/SfmDataLayer.hpp>
#include <SfmDataLayer/SfmPointRenderable.hpp>

/** @brief IRenderable drawing an SfmDataLayer by delegating to a point and a camera renderable. */
class SfmDataRenderable : public IRenderable
{
  public:
    /** @brief Forward the layer parameters, and the data when it changed, to the sub-renderables. */
    void sync(LayerItem* layer, SceneView* view) override;
    /** @brief Initialize both sub-renderables. */
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    /** @brief Prepare both sub-renderables. */
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    /** @brief Draw the landmarks, then the cameras. */
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    /** @brief Landmark renderer. */
    SfmPointRenderable _points;
    /** @brief Camera renderer. */
    SfmCameraRenderable _cameras;
};