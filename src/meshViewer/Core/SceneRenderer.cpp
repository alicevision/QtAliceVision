#include <Core/SceneRenderer.hpp>
#include <Core/SceneView.hpp>
#include <Core/LayerItem.hpp>
#include <Core/Picking.hpp>

#include <QMetaObject>
#include <QPointer>
#include <limits>

namespace {

struct LayerPickSelection
{
    QPointer<LayerItem> layer;
    LayerPickResult result;
};

LayerPickSelection pickClosestLayer(const QList<LayerItem*>& layers, const Ray& ray)
{
    LayerPickSelection bestSelection;
    bestSelection.result.distance = std::numeric_limits<float>::max();

    for (LayerItem* layer : layers)
    {
        if (!layer || !layer->visible() || !layer->picking() || !layer->canPick())
        {
            continue;
        }

        const LayerPickResult candidate = layer->pick(ray);
        if (candidate.hit && candidate.distance > 0.0f && candidate.distance < bestSelection.result.distance)
        {
            bestSelection.layer = layer;
            bestSelection.result = candidate;
        }
    }

    if (!bestSelection.result.hit)
    {
        return {};
    }

    return bestSelection;
}

}  // namespace

SceneRenderer::SceneRenderer() = default;
SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::updateRenderables(const QList<LayerItem*>& newLayers)
{
    std::unordered_map<LayerItem*, std::unique_ptr<IRenderable>> existing;
    existing.reserve(static_cast<size_t>(_layerItems.size()));
    for (int i = 0; i < _layerItems.size(); ++i)
    {
        existing.emplace(_layerItems[i], std::move(_renderables[i]));
    }

    _layerItems = newLayers;
    _renderables.clear();
    _renderables.reserve(static_cast<size_t>(newLayers.size()));

    for (LayerItem* layer : newLayers)
    {
        auto it = existing.find(layer);
        if (it != existing.end())
        {
            _renderables.push_back(std::move(it->second));
            existing.erase(it);
        }
        else
        {
            auto r = layer->createRenderable();
            if (r && _rhi && _rpDesc)
            {
                r->initialize(_rhi, _rpDesc);
            }
            _renderables.push_back(std::move(r));
        }
    }
}

void SceneRenderer::initialize(QRhiCommandBuffer* cb)
{
    Q_UNUSED(cb)

    _rhi = rhi();
    _rpDesc = renderTarget()->renderPassDescriptor();

    for (auto& r : _renderables)
    {
        if (r)
        {
            r->initialize(_rhi, _rpDesc);
        }
    }
}

void SceneRenderer::synchronize(QQuickRhiItem* item)
{
    SceneView* view = static_cast<SceneView*>(item);

    // Rebuild renderables whenever the layer list changes.
    const QList<LayerItem*>& newLayers = view->layerList();
    if (newLayers != _layerItems)
    {
        updateRenderables(newLayers);
    }

    _state.viewportWidth = static_cast<int>(view->width());
    _state.viewportHeight = static_cast<int>(view->height());

    for (int i = 0; i < _layerItems.size(); ++i)
    {
        if (_renderables[i])
        {
            _renderables[i]->sync(_layerItems[i], view);
        }
    }

    CameraInfo* cameraInfo = view->getCameraInfo();
    MotionInfo* motionInfo = view->getMotionInfo();

    cameraInfo->setBackendProjectionMatrix(_rhi ? _rhi->clipSpaceCorrMatrix() : QMatrix4x4{});

    double aspectRatio = view->height() > 0 ? static_cast<double>(view->width()) / static_cast<double>(view->height()) : 1.0f;

    const QMatrix4x4 model = motionInfo->getMatrix();
    const QMatrix4x4 proj = cameraInfo->getProjectionMatrix(aspectRatio);
    const QMatrix4x4 ortho = cameraInfo->getOrthogonalMatrix(aspectRatio);

    _state.orthogonalMatrix = ortho;
    _state.viewProjection = proj * model;
    _state.normalMatrix = model.inverted().transposed();
    _state.projectionScaleY = proj(1, 1);

    const PendingPickRequest pickRequest = view->takePendingPickRequest();
    if (pickRequest.pending && _state.viewportWidth > 0 && _state.viewportHeight > 0)
    {
        const Ray ray =
          unprojectRay(proj, model, pickRequest.mousePos, static_cast<float>(_state.viewportWidth), static_cast<float>(_state.viewportHeight));

        LayerPickSelection selection = pickClosestLayer(_layerItems, ray);
        selection.result.userCode = pickRequest.userCode;

        QMetaObject::invokeMethod(view, [view, selection]() { view->applyLayerPick(selection.layer, selection.result); }, Qt::QueuedConnection);
    }
}

void SceneRenderer::render(QRhiCommandBuffer* cb)
{
    QRhiResourceUpdateBatch* batch = _rhi->nextResourceUpdateBatch();
    for (auto& r : _renderables)
    {
        if (r)
        {
            r->prepare(batch, _state);
        }
    }

    cb->resourceUpdate(batch);

    const QColor clearColor = QColor::fromRgbF(0.15f, 0.15f, 0.15f, 1.0f);
    cb->beginPass(renderTarget(), clearColor, {1.0f, 0});

    for (int i = 0; i < static_cast<int>(_renderables.size()); ++i)
    {
        if (_renderables[i] && _layerItems[i]->visible() && _layerItems[i]->rendersInBackground())
        {
            _renderables[i]->render(cb, _state);
        }
    }

    for (int i = 0; i < static_cast<int>(_renderables.size()); ++i)
    {
        if (_layerItems[i]->rendersInBackground())
        {
            continue;
        }

        if (_layerItems[i]->rendersInForeground())
        {
            continue;
        }

        if (_renderables[i] && _layerItems[i]->visible())
        {
            _renderables[i]->render(cb, _state);
        }
    }

    for (int i = 0; i < static_cast<int>(_renderables.size()); ++i)
    {
        if (_renderables[i] && _layerItems[i]->visible() && _layerItems[i]->rendersInForeground())
        {
            _renderables[i]->render(cb, _state);
        }
    }

    cb->endPass();
}