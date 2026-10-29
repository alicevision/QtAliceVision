#pragma once

#include <QQuickRhiItem>
#include <rhi/qrhi.h>
#include <QList>
#include <QElapsedTimer>
#include <memory>
#include <vector>

#include <Core/IRenderable.hpp>
#include <Core/SceneState.hpp>

class LayerItem;
class SceneView;

/**
 * @brief QQuickRhiItemRenderer that orchestrates a set of IRenderable objects.
 *
 * Per-frame loop:
 *  - synchronize(): fills SceneState; rebuilds renderables when layers change;
 *                   calls IRenderable::sync() on each renderable.
 *  - initialize(): calls IRenderable::initialize() on each renderable.
 *  - render(): calls IRenderable::prepare() then IRenderable::render() within a single pass.
 *              Invisible layers (LayerItem::visible()==false) skip render() but not prepare().
 */
class SceneRenderer : public QQuickRhiItemRenderer
{
  public:
    /** @brief Build a renderer with no renderables; they are created on the first synchronize(). */
    SceneRenderer();
    /** @brief Destroy the renderer and all its renderables. */
    ~SceneRenderer() override;

    /**
     * @brief Capture the QRhi and render pass descriptor, then initialize every renderable.
     * @param[in] cb Unused command buffer.
     */
    void initialize(QRhiCommandBuffer* cb) override;
    /**
     * @brief Synchronize with the SceneView while the GUI thread is blocked.
     *
     * Updates the renderables, computes the SceneState from the camera and motion, and resolves
     * the pending pick request (the result is applied asynchronously on the GUI thread).
     *
     * @param[in] item The SceneView owning this renderer.
     */
    void synchronize(QQuickRhiItem* item) override;
    /**
     * @brief Prepare all renderables then draw them in a single pass.
     *
     * Draw order: background layers, regular layers, then foreground layers.
     *
     * @param[in] cb Command buffer to record into.
     */
    void render(QRhiCommandBuffer* cb) override;

  private:
    /**
     * @brief Incrementally update renderables to match @p newLayers.
     *
     * Renderables for layers that remain in the list are preserved; only
     * new layers get a freshly created renderable and removed layers have
     * their renderable destroyed. This avoids re-uploading GPU resources
     * for unchanged layers.
     */
    void updateRenderables(const QList<LayerItem*>& newLayers);

    /** @brief Rendering hardware interface, set by initialize(). */
    QRhi* _rhi = nullptr;
    /** @brief Render pass descriptor of the item render target. */
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    /** @brief Per-frame state shared with every renderable. */
    SceneState _state;
    /** @brief Layers mirrored from the SceneView, in draw order. */
    QList<LayerItem*> _layerItems;
    /** @brief Renderables, index-aligned with _layerItems. */
    std::vector<std::unique_ptr<IRenderable>> _renderables;
};
