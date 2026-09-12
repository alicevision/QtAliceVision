#include <MeshLayer/MeshLayer.hpp>
#include <MeshLayer/MeshRenderable.hpp>

MeshLayer::MeshLayer(QObject* parent)
  : LayerItem(parent)
{
    connect(this, &LayerItem::opacityChanged, this, [this]() { _opacityDirty = true; });
}

void MeshLayer::setMesh(MeshObject* mesh)
{
    if (_mesh == mesh)
        return;

    if (_meshConnection)
        QObject::disconnect(_meshConnection);

    _mesh = mesh;

    if (_mesh)
    {
        _meshConnection = connect(_mesh, &MeshObject::dataReady, this, &MeshLayer::onMeshDataReady);
        // If data is already available, treat it as immediately dirty.
        if (_mesh->valid())
            onMeshDataReady();
    }

    emit meshChanged();
}

void MeshLayer::setSelection(const QVector3D& selection)
{
    if (_selection == selection)
    {
        return;
    }

    _selection = selection;
    _selectionDirty = true;

    emit selectionChanged();
    emit dataReady();
}

void MeshLayer::setWireframeMode(WireframeMode mode)
{
    if (_wireframeMode == mode)
        return;
    _wireframeMode = mode;
    _wireframeDirty = true;
    emit wireframeModeChanged();
    emit dataReady();
}

void MeshLayer::setShadingMode(ShadingMode mode)
{
    if (_shadingMode == mode)
        return;
    _shadingMode = mode;
    _shadingDirty = true;
    emit shadingModeChanged();
    emit dataReady();
}

void MeshLayer::onMeshDataReady()
{
    _meshDirty = true;
    _wireframeDirty = true;
    _shadingDirty = true;
    _picker.buildBVH(_mesh->meshData());
    emit dataReady();
}

std::unique_ptr<IRenderable> MeshLayer::createRenderable() const
{
    return std::make_unique<MeshRenderable>();
}

LayerPickResult MeshLayer::pick(const Ray& ray) const
{
    if (!_mesh || !_mesh->valid() || !_picker.isReady())
    {
        return {};
    }

    const HitResult hit = _picker.pick(ray);
    if (!hit.hit)
    {
        return {};
    }

    const QVector3D direction = ray.direction.normalized();
    return {0, true, hit.distance, ray.origin + direction * hit.distance};
}

void MeshLayer::applyPickResult(const LayerPickResult& result)
{
    if (!result.hit)
    {
        return;
    }

    setSelection(result.worldPoint);
}

void MeshLayer::clearPick()
{}