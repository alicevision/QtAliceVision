#include <SfmDataLayer/SfmDataLayer.hpp>
#include <SfmDataLayer/SfmDataRenderable.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>

namespace {}  // namespace

SfmDataLayer::SfmDataLayer(QObject* parent)
  : LayerItem(parent)
{}

void SfmDataLayer::setSfmData(SfmDataObject* sfmData)
{
    if (_sfmData == sfmData)
        return;

    if (_sfmDataConnection)
        QObject::disconnect(_sfmDataConnection);
    if (_sfmDataLimitConnection)
        QObject::disconnect(_sfmDataLimitConnection);

    _sfmData = sfmData;

    if (_sfmData)
    {
        _sfmDataConnection = connect(_sfmData, &SfmDataObject::dataReady, this, &SfmDataLayer::onSfmDataReady);
        _sfmDataLimitConnection = connect(_sfmData, &SfmDataObject::limitResectionIdChanged, this, &SfmDataLayer::onLimitResectionIdChanged);

        // If data is already available, treat it as immediately dirty.
        if (_sfmData->valid())
        {
            onSfmDataReady();
        }
    }

    emit sfmDataChanged();
}

void SfmDataLayer::setPointSize(float pointSize)
{
    const float clamped = std::max(pointSize, 0.0f);

    _pointSize = clamped;
    if (_sfmData && _sfmData->valid())
    {
        rebuildPicker();
        _dataDirty = true;
        emit dataReady();
    }
    emit pointSizeChanged();
}

void SfmDataLayer::setCameraSize(float cameraSize)
{
    const float clamped = std::max(cameraSize, 0.0f);

    _cameraSize = clamped;
    if (_sfmData && _sfmData->valid())
    {
        rebuildPicker();
        _dataDirty = true;
        emit dataReady();
    }
    emit cameraSizeChanged();
}

void SfmDataLayer::onSfmDataReady()
{
    _dataDirty = true;
    rebuildPicker();
    emit dataReady();
}

void SfmDataLayer::onLimitResectionIdChanged()
{
    _picker.clearSelectedObject();
    _dataDirty = true;
    rebuildPicker();
    emit dataReady();
}

std::vector<SfmDataCameraInstance> SfmDataLayer::visibleCameras() const
{
    std::vector<SfmDataCameraInstance> result;
    if (_sfmData && _sfmData->valid())
    {
        const SfmDataContent& c = _sfmData->content();
        const quint32 limit = _sfmData->limitResectionId();
        result.reserve(c.cameras.size());
        std::copy_if(c.cameras.begin(), c.cameras.end(), std::back_inserter(result), [limit](const SfmDataCameraInstance& camera) {
            return ((camera.resectionId <= limit) || (camera.resectionId == aliceVision::UndefinedIndexT));
        });
    }
    return result;
}

void SfmDataLayer::rebuildPicker()
{
    if (_sfmData && _sfmData->valid())
    {
        const SfmDataContent& c = _sfmData->content();
        _picker.build(c.points, _pointSize, visibleCameras(), _cameraSize);
    }
    else
    {
        _picker.build({}, 0.0f, {}, 1.0f);
    }
}

std::unique_ptr<IRenderable> SfmDataLayer::createRenderable() const
{
    return std::make_unique<SfmDataRenderable>();
}

LayerPickResult SfmDataLayer::pick(const Ray& ray) const
{
    if (!_picker.isReady())
        return {};

    const SfmDataPickHit hit = _picker.pick(ray);
    if (!hit.hit)
        return {};

    _lastHit = hit;

    return {0, true, hit.distance, hit.worldPoint};
}

std::optional<size_t> SfmDataLayer::selectedCameraIndex() const
{
    const SfmDataPickHit& selectedObject = _picker.selectedObject();
    if (!selectedObject.hit || selectedObject.kind != SfmDataPickHit::Kind::Camera)
        return std::nullopt;

    return selectedObject.index;
}

void SfmDataLayer::applyPickResult(const LayerPickResult& result)
{
    Q_UNUSED(result)
    _picker.setSelectedObject(_lastHit);
}

void SfmDataLayer::clearPick()
{
    _picker.clearSelectedObject();
    _lastHit.kind = SfmDataPickHit::Kind::None;
}