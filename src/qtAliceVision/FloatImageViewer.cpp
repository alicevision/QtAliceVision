#include "FloatImageViewer.hpp"
#include "FloatTexture.hpp"

#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGSimpleMaterialShader>
#include <QSGTexture>
#include <QThreadPool>

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
    connect(this, &FloatImageViewer::gammaChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::gainChanged, this, &FloatImageViewer::update);

    connect(this, &FloatImageViewer::textureSizeChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::sourceSizeChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::imageChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::sourceChanged, this, &FloatImageViewer::reload);

    connect(this, &FloatImageViewer::channelModeChanged, this, &FloatImageViewer::update);

    connect(this, &FloatImageViewer::downscaleLevelChanged, this, &FloatImageViewer::reload);

    connect(&_surface, &Surface::gridColorChanged, this, &FloatImageViewer::update);
    connect(&_surface, &Surface::gridOpacityChanged, this, &FloatImageViewer::update);
    connect(&_surface, &Surface::displayGridChanged, this, &FloatImageViewer::update);

    connect(&_surface, &Surface::mouseOverChanged, this, &FloatImageViewer::update);
    connect(&_surface, &Surface::viewerTypeChanged, this, &FloatImageViewer::update);

    connect(&_surface, &Surface::subdivisionsChanged, this, &FloatImageViewer::update);
    connect(&_surface, &Surface::verticesChanged, this, &FloatImageViewer::update);

    connect(&_singleImageLoader, &imgserve::SingleImageLoader::requestHandled, this, &FloatImageViewer::reload);
    connect(&_sequenceCache, &imgserve::SequenceCache::requestHandled, this, &FloatImageViewer::reload);
    connect(&_sequenceCache, &imgserve::SequenceCache::contentChanged, this, &FloatImageViewer::reload);
    connect(this, &FloatImageViewer::useSequenceChanged, this, &FloatImageViewer::reload);
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

void FloatImageViewer::setTargetSize(int size)
{
    _sequenceCache.setTargetSize(size);
    Q_EMIT targetSizeChanged();
}

QVariantList FloatImageViewer::getCachedFrames() const { return _sequenceCache.getCachedFrames(); }

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
    else if (response.error != imgserve::LoadingStatus::SUCCESSFUL)
    {
        _image.reset();
        _imageChanged = true;
        Q_EMIT imageChanged();
        setLoading(false);
        switch (response.error)
        {
            case imgserve::LoadingStatus::MISSING_FILE:
                setStatus(EStatus::MISSING_FILE);
                break;
            case imgserve::LoadingStatus::ERROR:
            default:
                setStatus(EStatus::ERROR);
                break;
        }
    }
    else if (_outdated)
    {
        qWarning() << "[QtAliceVision] The loading status has not been updated since the last reload. Something wrong might have happened.";
        setStatus(EStatus::OUTDATED_LOADING);
    }
    else
    {
        setLoading(true);
    }

    Q_EMIT cachedFramesChanged();
}

void FloatImageViewer::playback(bool active)
{
    // Turn off interactive prefetching when playback is ON
    _sequenceCache.setInteractivePrefetching(!active);
}

QVector4D FloatImageViewer::pixelValueAt(int x, int y)
{
    if (!_image)
    {
        // qInfo() << "[QtAliceVision] FloatImageViewer::pixelValueAt(" << x << ", " << y << ") => no valid image";
        return QVector4D(0.0, 0.0, 0.0, 0.0);
    }
    else if (x < 0 || x >= _image->Width() || y < 0 || y >= _image->Height())
    {
        // qInfo() << "[QtAliceVision] FloatImageViewer::pixelValueAt(" << x << ", " << y << ") => out of range";
        return QVector4D(0.0, 0.0, 0.0, 0.0);
    }
    aliceVision::image::RGBAfColor color = (*_image)(y, x);
    // qInfo() << "[QtAliceVision] FloatImageViewer::pixelValueAt(" << x << ", " << y << ") => valid pixel: " <<
    // color(0) << ", " << color(1) << ", " << color(2) << ", " << color(3);
    return QVector4D(color(0), color(1), color(2), color(3));
}

QSGNode* FloatImageViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
{
    (void)data;  // Fix "unused parameter" warnings; should be replaced by [[maybe_unused]] when C++17 is supported
    QVector4D channelOrder(0.f, 1.f, 2.f, 3.f);

    QSGGeometryNode* root = static_cast<QSGGeometryNode*>(oldNode);
    QSGSimpleMaterial<ShaderData>* material = nullptr;

    QSGGeometry* geometryLine = nullptr;

    if (!root)
    {
        root = new QSGGeometryNode;
        auto geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), _surface.vertexCount(), _surface.indexCount());
        geometry->setDrawingMode(GL_TRIANGLES);
        geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
        geometry->setVertexDataPattern(QSGGeometry::StaticPattern);
        root->setGeometry(geometry);
        root->setFlags(QSGNode::OwnsGeometry);

        material = ImageViewerShader::createMaterial();
        root->setMaterial(material);
        root->setFlags(QSGNode::OwnsMaterial);
        {
            /* Geometry and Material for the Grid */
            auto node = new QSGGeometryNode;
            auto gridMaterial = new QSGFlatColorMaterial;
            gridMaterial->setColor(_surface.getGridColor());
            {
                // Vertexcount of the grid is equal to indexCount of the image
                geometryLine = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), _surface.indexCount());
                geometryLine->setDrawingMode(GL_LINES);
                geometryLine->setLineWidth(2);

                node->setGeometry(geometryLine);
                node->setFlags(QSGNode::OwnsGeometry);
                node->setMaterial(gridMaterial);
                node->setFlags(QSGNode::OwnsMaterial);
            }
            root->appendChildNode(node);
        }
    }
    else
    {
        _createRoot = false;
        material = static_cast<QSGSimpleMaterial<ShaderData>*>(root->material());

        QSGGeometryNode* rootGrid = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(0));
        auto mat = static_cast<QSGFlatColorMaterial*>(rootGrid->activeMaterial());
        mat->setColor(_surface.getGridColor());
        geometryLine = rootGrid->geometry();
    }

    if (_surface.hasSubdivisionsChanged())
    {
        // Re size grid
        if (geometryLine)
        {
            // Vertexcount of the grid is equal to indexCount of the image
            geometryLine->allocate(_surface.indexCount());
            root->childAtIndex(0)->markDirty(QSGNode::DirtyGeometry);
        }
        // Re size root
        if (root)
        {
            root->geometry()->allocate(_surface.vertexCount(), _surface.indexCount());
            root->markDirty(QSGNode::DirtyGeometry);
        }
    }

    // enable Blending flag for transparency for RGBA
    material->setFlag(QSGMaterial::Blending, _channelMode == EChannelMode::RGBA);

    // change channelOrder according to channelMode
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
        case EChannelMode::RGB:
        case EChannelMode::RGBA:
        default:
            break;
    }

    bool updateGeometry = false;
    material->state()->gamma = _gamma;
    material->state()->gain = _gain;
    material->state()->channelOrder = channelOrder;

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

            // Crop the image to only display what is inside the fisheye circle
            const aliceVision::camera::Equidistant* intrinsicEquidistant = _surface.getIntrinsicEquidistant();
            if (_cropFisheye && intrinsicEquidistant)
            {
                const aliceVision::Vec3 fisheyeCircleParams(
                  intrinsicEquidistant->getCircleCenterX(), intrinsicEquidistant->getCircleCenterY(), intrinsicEquidistant->getCircleRadius());

                const double width = _image->Width() * pow(2.0, _downscaleLevel);
                const double height = _image->Height() * pow(2.0, _downscaleLevel);
                const double aspectRatio = (width > height) ? width / height : height / width;

                const double radiusInPercentage = (fisheyeCircleParams.z() / ((width > height) ? height : width)) * 2.0;

                // Radius is converted in uv coordinates (0, 0.5)
                const float radius = 0.5f * static_cast<float>(radiusInPercentage);

                material->state()->fisheyeCircleCoord =
                  QVector2D(static_cast<float>(fisheyeCircleParams.x() / width), static_cast<float>(fisheyeCircleParams.y() / height));
                material->state()->fisheyeCircleRadius = radius;
                material->state()->aspectRatio = static_cast<float>(aspectRatio);
            }
            else
            {
                material->state()->fisheyeCircleRadius = 0.0;
            }
        }
        material->state()->texture = std::move(texture);

        _imageChanged = false;

        if (_textureSize != newTextureSize)
        {
            _textureSize = newTextureSize;
            updateGeometry = true;
            Q_EMIT textureSizeChanged();
        }
    }

    const auto newBoundingRect = boundingRect();
    if (updateGeometry || _boundingRect != newBoundingRect)
    {
        _boundingRect = newBoundingRect;

        const double windowRatio = _boundingRect.width() / _boundingRect.height();
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

        QSGGeometry::updateTexturedRectGeometry(root->geometry(), geometryRect, QRectF(0, 0, 1, 1));
        root->markDirty(QSGNode::DirtyGeometry);
    }

    /*
     * Surface
     */
    if (root && !_createRoot && _image)
    {
        updatePaintSurface(root, material, geometryLine);
    }

    return root;
}

void FloatImageViewer::updatePaintSurface(QSGGeometryNode* root, QSGSimpleMaterial<ShaderData>* material, QSGGeometry* geometryLine)
{
    // Highlight
    if (_canBeHovered)
    {
        if (_surface.getMouseOver())
        {
            material->state()->gamma += 1.0f;
        }
        root->markDirty(QSGNode::DirtyMaterial);
    }

    // If vertices has changed, Re-Compute the grid
    if (_surface.hasVerticesChanged())
    {
        // Retrieve Vertices and Index Data
        QSGGeometry::TexturedPoint2D* vertices = root->geometry()->vertexDataAsTexturedPoint2D();
        quint16* indices = root->geometry()->indexDataAsUShort();

        // Update surface
        const QSize surfaceSize = _surface.isPanoramaViewerEnabled() ? _textureSize : _sourceSize;
        _surface.update(vertices, indices, surfaceSize, _downscaleLevel);

        root->geometry()->markIndexDataDirty();
        root->geometry()->markVertexDataDirty();
        root->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);

        // Fill the Surface vertices array
        _surface.fillVertices(vertices);
    }

    // Draw the grid if Distortion Viewer is enabled and Grid Mode is enabled
    _surface.getDisplayGrid() ? _surface.computeGrid(geometryLine) : _surface.removeGrid(geometryLine);

    root->childAtIndex(0)->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
}

}  // namespace qtAliceVision
