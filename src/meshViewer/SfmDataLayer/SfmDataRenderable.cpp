#include <SfmDataLayer/SfmDataRenderable.hpp>

#include <Core/LayerItem.hpp>

void SfmDataRenderable::sync(LayerItem* layer, SceneView* view)
{
    Q_UNUSED(view)

    auto* sfmLayer = qobject_cast<SfmDataLayer*>(layer);
    if (!sfmLayer)
    {
        return;
    }

    _cameras.setSelectedCamera(sfmLayer->selectedCameraIndex());
    _points.setPointRadius(sfmLayer->pointRadius());

    if (!sfmLayer->dataDirty())
    {
        return;
    }

    SfmDataObject* sfmData = sfmLayer->sfmData();
    if (!sfmData || !sfmData->valid())
    {
        sfmLayer->clearDataDirty();
        return;
    }

    const SfmDataContent& content = sfmData->content();
    _points.setData(content.points, sfmLayer->pointRadius());
    _cameras.setData(sfmLayer->visibleCameras(), sfmLayer->renderedCameraScale());

    sfmLayer->clearDataDirty();
}

void SfmDataRenderable::initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc)
{
    _points.initialize(rhi, rpDesc);
    _cameras.initialize(rhi, rpDesc);
}

void SfmDataRenderable::prepare(QRhiResourceUpdateBatch* batch, const SceneState& state)
{
    _points.prepare(batch, state);
    _cameras.prepare(batch, state);
}

void SfmDataRenderable::render(QRhiCommandBuffer* cb, const SceneState& state)
{
    _points.render(cb, state);
    _cameras.render(cb, state);
}