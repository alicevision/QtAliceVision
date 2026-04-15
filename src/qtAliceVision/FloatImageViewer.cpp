#include "FloatImageViewer.hpp"
#include "FloatTexture.hpp"

#include "FloatImageViewerMaterial.hpp"
#include "FloatImageViewerMaterialShader.hpp"
#include "FloatImageViewerNode.hpp"

#include <QSGGeometry>
#include <QSGTexture>
#include <QThreadPool>
#include <QPointer>

#include <aliceVision/camera/Equidistant.hpp>

#include <cmath>
#include <algorithm>
#include <vector>
#include <utility>

namespace qtAliceVision {


FloatImageViewer::FloatImageViewer(QQuickItem* parent)
  : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);

    // CONNECTS
    connect(this, &FloatImageViewer::gammaChanged, this, [this] {
        _gammaChanged = true;
        update();
    });
    connect(this, &FloatImageViewer::gainChanged, this, [this] {
        _gainChanged = true;
        update();
    });

    connect(this, &FloatImageViewer::textureSizeChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::sourceSizeChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::imageChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::sourceChanged, this, &FloatImageViewer::reload);

    connect(this, &FloatImageViewer::channelModeChanged, this, [this] {
        _channelModeChanged = true;
        update();
    });

    connect(this, &FloatImageViewer::downscaleLevelChanged, this, &FloatImageViewer::reload);

    connect(&_surface, &Surface::gridColorChanged, this, &FloatImageViewer::update);
    connect(&_surface, &Surface::gridOpacityChanged, this, &FloatImageViewer::update);
    connect(&_surface, &Surface::displayGridChanged, this, &FloatImageViewer::update);

    connect(&_surface, &Surface::mouseOverChanged, this, [this] {
        _mouseOverChanged = true;
        update();
    });
    connect(&_surface, &Surface::viewerTypeChanged, this, &FloatImageViewer::update);

    connect(&_surface, &Surface::subdivisionsChanged, this, &FloatImageViewer::update);
    connect(&_surface, &Surface::verticesChanged, this, &FloatImageViewer::update);

    connect(&_singleImageLoader, &imgserve::SingleImageLoader::requestHandled, this, &FloatImageViewer::reload);
    connect(&_sequenceCache, &imgserve::SequenceCache::requestHandled, this, &FloatImageViewer::reload);
    connect(this, &FloatImageViewer::useSequenceChanged, this, &FloatImageViewer::reload);
    connect(this, &FloatImageViewer::sequenceChanged, this, &FloatImageViewer::reload);
}

FloatImageViewer::~FloatImageViewer() {}

void FloatImageViewer::setStatus(EStatus status)
{
    if (_status == status)
    {
        return;
    }
    _status = status;
    Q_EMIT statusChanged();
}

// LOADING FUNCTIONS
void FloatImageViewer::setLoading(bool loading)
{
    if (_loading == loading)
    {
        return;
    }
    _loading = loading;
}

void FloatImageViewer::setSequence(const QVariantList& paths)
{
    _sequenceCache.setSequence(paths);
    Q_EMIT sequenceChanged();
}

void FloatImageViewer::setFetchingSequence(bool fetching)
{
    _sequenceCache.setPrefetching(fetching);
    Q_EMIT fetchingSequenceChanged();
}

bool FloatImageViewer::getFetchingSequence()
{
    return _sequenceCache.getPrefetching();
}

void FloatImageViewer::setResizeRatio(double ratio)
{
    ratio = std::clamp(ratio, 0.0, 1.0);
    ratio = std::ceil(ratio * 10.0) / 10.0;

    _sequenceCache.setResizeRatio(ratio);

    if (ratio != _clampedResizeRatio)
    {
        // If the clamped ratio has changed, then
        // We may need to reload the image with the correct resolution
        Q_EMIT sourceChanged();
    }

    _clampedResizeRatio = ratio;

    Q_EMIT resizeRatioChanged();
}

double FloatImageViewer::getResizeRatio()
{
    return _clampedResizeRatio;
}

void FloatImageViewer::setMemoryLimit(int memoryLimit)
{
    const int clampedMemoryLimit = std::max(0, memoryLimit);
    _sequenceCache.setMemoryLimit(static_cast<std::size_t>(clampedMemoryLimit));
    Q_EMIT memoryLimitChanged();
}

int FloatImageViewer::getMemoryLimit()
{
    return static_cast<int>(_sequenceCache.getMemoryLimit());
}

QVariantList FloatImageViewer::getCachedFrames() const
{ 
    return _sequenceCache.getCachedFrames(); 
}

QPointF FloatImageViewer::getRamInfo() const 
{ 
    return _sequenceCache.getRamInfo(); 
}

void FloatImageViewer::reload()
{
    if (_clearBeforeLoad)
    {
        _image.reset();
        _imageChanged = true;
        Q_EMIT imageChanged();
    }

    _outdated = false;
    if (_loading)
    {
        _outdated = true;
    }

    if (!_source.isValid())
    {
        _image.reset();
        _imageChanged = true;
        _surface.clearVertices();
        _surface.verticesChanged();
        Q_EMIT imageChanged();
        return;
    }

    // Send request
    imgserve::RequestData reqData;
    reqData.path = _source.toLocalFile().toUtf8().toStdString();
    reqData.downscale = 1 << _downscaleLevel;

    imgserve::ResponseData response = _useSequence ? _sequenceCache.request(reqData) : _singleImageLoader.request(reqData);

    if (response.img)
    {
        setLoading(false);
        setStatus(EStatus::NONE);

        _surface.setVerticesChanged(true);
        _surface.setNeedToUseIntrinsic(true);
        _image = response.img;
        _imageChanged = true;
        Q_EMIT imageChanged();

        _sourceSize = response.dim;
        Q_EMIT sourceSizeChanged();

        _metadata = response.metadata;
        Q_EMIT metadataChanged();
    }
    else if (response.error == imgserve::LoadingStatus::UNDEFINED)
    {
        setLoading(true);
        setStatus(EStatus::LOADING);
    }
    else if (response.error == imgserve::LoadingStatus::MISSING_FILE)
    {
        _image.reset();
        setStatus(EStatus::MISSING_FILE);
    }
    else if (response.error == imgserve::LoadingStatus::LOADING_ERROR)
    {
        _image.reset();
        setStatus(EStatus::LOADING_ERROR);
    }
    else if (_outdated)
    {
        qWarning()
          << "[QtAliceVision] FloatImageViewer: The loading status has not been updated since the last reload. Something wrong might have happened.";
        setStatus(EStatus::OUTDATED_LOADING);
    }
    Q_EMIT cachedFramesChanged();
}

void FloatImageViewer::playback(bool /*active*/) {}

void FloatImageViewer::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    _geometryChanged = true;
}

QVector4D FloatImageViewer::pixelValueAt(int x, int y)
{
    if (_useSequence)
    {
        x = int(std::ceil(double(x) * _clampedResizeRatio));
        y = int(std::ceil(double(y) * _clampedResizeRatio));
    }

    if (!_image)
    {
        qDebug() << "[QtAliceVision] FloatImageViewer::pixelValueAt(" << x << ", " << y << ") => no valid image";
        return QVector4D(0.0, 0.0, 0.0, 0.0);
    }
    else if (x < 0 || x >= _image->width() || y < 0 || y >= _image->height())
    {
        qDebug() << "[QtAliceVision] FloatImageViewer::pixelValueAt(" << x << ", " << y << ") => out of range";
        return QVector4D(0.0, 0.0, 0.0, 0.0);
    }
    aliceVision::image::RGBAfColor color = (*_image)(y, x);
    qDebug() << "[QtAliceVision] FloatImageViewer::pixelValueAt(" << x << ", " << y << ") => valid pixel: " << color(0) << ", " << color(1) << ", "
             << color(2) << ", " << color(3);
    return QVector4D(color(0), color(1), color(2), color(3));
}

QSGNode* FloatImageViewer::updatePaintNode(QSGNode* oldNode, [[maybe_unused]] QQuickItem::UpdatePaintNodeData* data)
{
    auto* node = static_cast<FloatImageViewerNode*>(oldNode);
    bool isNewNode = false;
    if (!node)
    {
        node = new FloatImageViewerNode(_surface.vertexCount(), _surface.indexCount());
        isNewNode = true;
    }
    else if (_surface.hasSubdivisionsChanged())
    {
        node->setSubdivisions(_surface.vertexCount(), _surface.indexCount());
    }

    node->setGridColor(_surface.getGridColor());

    if (_imageChanged)
    {
        QSize newTextureSize;
        auto texture = std::make_unique<FloatTexture>();
        if (_image)
        {
            texture->setImage(_image);
            texture->setFiltering(QSGTexture::Nearest);
            texture->setHorizontalWrapMode(QSGTexture::Repeat);
            texture->setVerticalWrapMode(QSGTexture::Repeat);
            newTextureSize = texture->textureSize();
            
            // commitTextureOperations() runs on the render thread after this function returns
            // and may downscale the image to fit GPU limits, changing the texture size.
            // Post the updated size back to the GUI thread so the signal fires correctly.
            QPointer<FloatImageViewer> weakThis(this);
            texture->setOnCommit([weakThis](QSize committedSize) {
                if (!weakThis)
                {
                    return;
                }

                QMetaObject::invokeMethod(weakThis, [weakThis, committedSize]() {
                    if (!weakThis)
                    {
                        return;
                    }

                    if (weakThis->_textureSize != committedSize)
                    {
                        weakThis->_textureSize = committedSize;
                        weakThis->_geometryChanged = true;
                        Q_EMIT weakThis->textureSizeChanged();
                    }
                }, Qt::QueuedConnection);
            });

            // Crop the image to only display what is inside the fisheye circle
            const aliceVision::camera::Equidistant* intrinsicEquidistant = _surface.getIntrinsicEquidistant();
            if (_cropFisheye && intrinsicEquidistant)
            {
                const aliceVision::Vec3 fisheyeCircleParams(intrinsicEquidistant->getCircleCenterX(), intrinsicEquidistant->getCircleCenterY(), intrinsicEquidistant->getCircleRadius());

                const double width = _image->width() * pow(2.0, _downscaleLevel);
                const double height = _image->height() * pow(2.0, _downscaleLevel);
                const double aspectRatio = (width > height) ? width / height : height / width;

                const double radiusInPercentage = (fisheyeCircleParams.z() / ((width > height) ? height : width)) * 2.0;

                // Radius is converted in uv coordinates (0, 0.5)
                const double radius = 0.5 * (radiusInPercentage);

                node->setFisheye(
                  static_cast<float>(aspectRatio),
                  static_cast<float>(radius),
                  QVector2D(static_cast<float>(fisheyeCircleParams.x() / width), static_cast<float>(fisheyeCircleParams.y() / height)));
            }
            else
            {
                node->resetFisheye();
            }
        }
        node->setTexture(std::move(texture));

        if (_textureSize != newTextureSize)
        {
            _textureSize = newTextureSize;
            _geometryChanged = true;
            Q_EMIT textureSizeChanged();
        }
    }
    _imageChanged = false;

    const auto newBoundingRect = boundingRect();
    if (_geometryChanged || _boundingRect != newBoundingRect)
    {
        _boundingRect = newBoundingRect;

        const float windowRatio = static_cast<float>(_boundingRect.width()) / static_cast<float>(_boundingRect.height());
        const float textureRatio = static_cast<float>(_textureSize.width()) / static_cast<float>(_textureSize.height());
        QRectF geometryRect = _boundingRect;
        if (windowRatio > textureRatio)
        {
            geometryRect.setWidth(geometryRect.height() * textureRatio);
        }
        else
        {
            geometryRect.setHeight(geometryRect.width() / textureRatio);
        }
        geometryRect.moveCenter(_boundingRect.center());

        static const int MARGIN = 0;
        geometryRect = geometryRect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);

        node->setRect(geometryRect);
    }
    _geometryChanged = false;

    if (isNewNode || _gammaChanged)
    {
        node->setGamma(_gamma);
    }
    _gammaChanged = false;

    if (isNewNode || _gainChanged)
    {
        node->setGain(_gain);
    }
    _gainChanged = false;

    if (isNewNode || _channelModeChanged)
    {
        QVector4D channelOrder(0.f, 1.f, 2.f, 3.f);
        switch (_channelMode)
        {
            case EChannelMode::R:
                channelOrder = QVector4D(0.f, 0.f, 0.f, -1.f);
                break;
            case EChannelMode::G:
                channelOrder = QVector4D(1.f, 1.f, 1.f, -1.f);
                break;
            case EChannelMode::B:
                channelOrder = QVector4D(2.f, 2.f, 2.f, -1.f);
                break;
            case EChannelMode::A:
                channelOrder = QVector4D(3.f, 3.f, 3.f, -1.f);
                break;
            default:
                break;
        }
        node->setChannelOrder(channelOrder);
        node->setBlending(_channelMode == EChannelMode::RGBA);
    }
    _channelModeChanged = false;

    if (!isNewNode && _image)
    {
        node->updatePaintSurface(_surface,
                                 _surface.isPanoramaViewerEnabled() ? _textureSize : _sourceSize,
                                 _downscaleLevel,
                                 _canBeHovered,
                                 !_surface.getMouseOver() && _mouseOverChanged);
    }
    _mouseOverChanged = false;
    return node;
}

}  // namespace qtAliceVision
