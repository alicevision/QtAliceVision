#include "PanoramaViewer.hpp"
#include "FloatImageViewer.hpp"
#include "FloatTexture.hpp"
#include "ShaderImageViewer.hpp"

#include <QSGGeometryNode>
#include <QSGGeometry>
#include <QSGSimpleMaterial>
#include <QSGSimpleMaterialShader>
#include <QSGTexture>
#include <QThreadPool>

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/resampling.hpp>
#include <aliceVision/image/io.hpp>
#include <aliceVision/camera/cameraUndistortImage.hpp>

#include <aliceVision/system/MemoryInfo.hpp>


namespace qtAliceVision
{
PanoramaViewer::PanoramaViewer(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);
    connect(this, &PanoramaViewer::sourceSizeChanged, this, &PanoramaViewer::update);
    connect(this, &PanoramaViewer::downscaleChanged, this, &PanoramaViewer::update);
    connect(this, &PanoramaViewer::sfmDataChanged, this, &PanoramaViewer::msfmDataUpdate);
}

PanoramaViewer::~PanoramaViewer()
{
}

void PanoramaViewer::computeDownscale()
{
    if (!_msfmData || !_sfmLoaded)
        return;

    int totalSizeImages = 0;
    for (const auto& view : _msfmData->rawData().getViews())
        totalSizeImages += int(((view.second->getWidth() * view.second->getHeight()) * 4) / std::pow(10, 6));

    // Downscale = 4 by default
    _downscale = 4;
    for (size_t i = 0; i < _downscale; i++) 
        totalSizeImages *= 0.5;

    // ensure it fits in RAM memory
    aliceVision::system::MemoryInfo memInfo = aliceVision::system::getMemoryInfo();
    const int freeRam = int(memInfo.freeRam / std::pow(2, 20));
    while (totalSizeImages > freeRam * 0.5)
    {
        _downscale++;
        totalSizeImages *= 0.5;
    }

    Q_EMIT downscaleReady();
}

QSGNode* PanoramaViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
{
    QSGGeometryNode* root = static_cast<QSGGeometryNode*>(oldNode);
    return root;
}

void PanoramaViewer::setMSfmData(MSfMData* sfmData)
{
    _sfmLoaded = false;

    if (_msfmData == sfmData || sfmData == nullptr)
        return;

    if (_msfmData != nullptr)
    {
        disconnect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    _msfmData = sfmData;
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

    qWarning() << "PANO SET SFM" << sfmData->getSfmDataPath();

    //_sfmLoaded = true;
    //Q_EMIT sfmDataChanged();
}

}  // qtAliceVision
