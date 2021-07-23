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
#include <QSGFlatColorMaterial>

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/resampling.hpp>
#include <aliceVision/image/io.hpp>
#include <aliceVision/camera/cameraUndistortImage.hpp>

#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/sfmDataIO/sfmDataIO.hpp>
#include <aliceVision/system/MemoryInfo.hpp>


namespace qtAliceVision
{
PanoramaViewer::PanoramaViewer(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);
    connect(this, &PanoramaViewer::sourceSizeChanged, this, &PanoramaViewer::update);
    connect(this, &PanoramaViewer::imagesDataChanged, this, &PanoramaViewer::update);
    connect(this, &PanoramaViewer::downscaleChanged, this, &PanoramaViewer::update);
    
    connect(this, &PanoramaViewer::sfmDataChanged, this, &PanoramaViewer::computeInputImages);
}

PanoramaViewer::~PanoramaViewer()
{
}

void PanoramaViewer::computeInputImages()
{
    // Loop on Views and insert (path, id)
    int totalSizeImages = 0;
    for (const auto& view : _msfmData->rawData().getViews())
    {
        QString path = QString::fromUtf8(view.second->getImagePath().c_str());
        QVariant id = view.second->getViewId();
        _imagesData.insert(path, id);

        totalSizeImages += int(((view.second->getWidth() * view.second->getHeight()) * 4) / std::pow(10, 6));
    }

    // Downscale == 4 by default
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

    Q_EMIT imagesDataChanged(_imagesData);
}

QSGNode* PanoramaViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
{
    QSGGeometryNode* root = static_cast<QSGGeometryNode*>(oldNode);
    return root;
}

void PanoramaViewer::setMSfmData(MSfMData* sfmData)
{
    _sfmLoaded = false;

    if (sfmData == nullptr)
    {
        _msfmData = sfmData;
        return;
    }

    if (_msfmData == sfmData)
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
        qWarning() << "[QtAliceVision] setMSfmData: SfMData is not ready: " << _msfmData->status();
        return;
    }
    if (_msfmData->rawData().getViews().empty())
    {
        qWarning() << "[QtAliceVision] setMSfmData: SfMData is empty";
        return;
    }

    _sfmLoaded = true;
    Q_EMIT sfmDataChanged();
}

}  // qtAliceVision
