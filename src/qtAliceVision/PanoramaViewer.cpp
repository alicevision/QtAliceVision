#include "PanoramaViewer.hpp"
#include "FloatImageViewer.hpp"
#include "FloatTexture.hpp"
#include "ShaderImageViewer.hpp"

#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGSimpleMaterial>
#include <QSGSimpleMaterialShader>
#include <QSGTexture>
#include <QThreadPool>

#include <aliceVision/camera/cameraUndistortImage.hpp>
#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/io.hpp>
#include <aliceVision/image/resampling.hpp>

#include <aliceVision/system/MemoryInfo.hpp>

namespace qtAliceVision {
PanoramaViewer::PanoramaViewer(QQuickItem* parent)
  : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);
    connect(this, &PanoramaViewer::sourceSizeChanged, this, &PanoramaViewer::update);
    connect(this, &PanoramaViewer::downscaleChanged, this, &PanoramaViewer::update);
    connect(this, &PanoramaViewer::sfmDataChanged, this, &PanoramaViewer::msfmDataUpdate);
}

PanoramaViewer::~PanoramaViewer() {}

void PanoramaViewer::computeDownscale()
{
    if (!_msfmData || _msfmData->status() != MSfMData::Status::Ready)
        return;

    int totalSizeImages = 0;
    for (const auto& view : _msfmData->rawData().getViews())
        totalSizeImages += int(static_cast<double>((view.second->getImage().getWidth() * view.second->getImage().getHeight()) * 4) / std::pow(10, 6));

    // Downscale = 4 by default
    _downscale = 4;
    for (int i = 0; i < _downscale; i++)
        totalSizeImages = static_cast<int>(totalSizeImages * 0.5);

    // ensure it fits in RAM memory
    aliceVision::system::MemoryInfo memInfo = aliceVision::system::getMemoryInfo();
    const int freeRam = int(static_cast<double>(memInfo.freeRam) / std::pow(2, 20));
    while (totalSizeImages > freeRam * 0.5)
    {
        _downscale++;
        totalSizeImages = static_cast<int>(totalSizeImages * 0.5);
    }

    Q_EMIT downscaleReady();
}

QSGNode* PanoramaViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
{
    (void)data;  // Fix "unused parameter" warnings; should be replaced by [[maybe_unused]] when C++17 is supported
    QSGGeometryNode* root = static_cast<QSGGeometryNode*>(oldNode);
    return root;
}

void PanoramaViewer::setMSfmData(MSfMData* sfmData)
{
    if (_msfmData == sfmData)
        return;

    if (_msfmData != nullptr)
    {
        disconnect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    _msfmData = sfmData;

    if (!_msfmData)
        return;

    if (_msfmData != nullptr)
    {
        connect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }

    if (_msfmData->status() != MSfMData::Ready)
    {
        qWarning() << "[QtAliceVision] PANORAMA setMSfmData: SfMData is not ready: " << _msfmData->status();
        return;
    }
    if (_msfmData->rawData().getViews().empty())
    {
        qWarning() << "[QtAliceVision] PANORAMA setMSfmData: SfMData is empty";
        return;
    }
}

}  // namespace qtAliceVision
