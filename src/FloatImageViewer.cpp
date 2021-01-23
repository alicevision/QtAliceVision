#include "FloatImageViewer.hpp"
#include "FloatTexture.hpp"
#include "ShaderImageViewer.hpp"

#include <QSGGeometryNode>
#include <QSGGeometry>
#include <QSGSimpleMaterial>
#include <QSGSimpleMaterialShader>
#include <QSGFlatColorMaterial>
#include <QSGTexture>
#include <QThreadPool>

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/resampling.hpp>
#include <aliceVision/image/io.hpp>
#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/convertion.hpp>

namespace qtAliceVision
{

namespace
{
    enum class RotateAngle
    {
        CW_90,
        CW_180,
        CW_270,
    };

    void copyPixelRotate90(FloatImage& dst, const FloatImage& src, int x, int y)
    {
        dst(x, src.Height() - y - 1) = src(y, x);
    }

    void copyPixelRotate270(FloatImage& dst, const FloatImage& src, int x, int y)
    {
        dst(src.Width() - x - 1, y) = src(y, x);
    }

    void copyPixelRotate180(FloatImage& dst, const FloatImage& src, int x, int y)
    {
        dst(src.Height() - y - 1, src.Width() - x - 1) = src(y, x);
    }

    template <class T>
    void forEach(FloatImage& dstImage, const FloatImage& srcImage, T&& f)
    {
        for (int y = 0; y != srcImage.Height(); ++y)
        {
            for (int x = 0; x != srcImage.Width(); ++x)
            {
                f(dstImage, srcImage, x, y);
            }
        }
    }

    void rotate(FloatImage& image, RotateAngle angle)
    {
        switch (angle)
        {
        case RotateAngle::CW_90:
        {
            FloatImage tmp;
            tmp.resize(image.Height(), image.Width(), false);
            forEach(tmp, image, &copyPixelRotate90);
            image = std::move(tmp);
            break;
        }
        case RotateAngle::CW_180:
        {
            // in place
            forEach(image, image, &copyPixelRotate180);
            break;
        }
        case RotateAngle::CW_270:
        {
            FloatImage tmp;
            tmp.resize(image.Height(), image.Width(), false);
            forEach(tmp, image, &copyPixelRotate270);
            image = std::move(tmp);
            break;
        }
        }
    }
}

FloatImageIORunnable::FloatImageIORunnable(const QUrl& path, QObject* parent)
    : QObject(parent)
    , _path(path)
{}

void FloatImageIORunnable::run()
{
    using namespace aliceVision;
    QSharedPointer<FloatImage> result;
    QSize sourceSize(0, 0);
    QVariantMap qmetadata;

    try
    {
        const auto path = _path.toLocalFile().toUtf8().toStdString();
        FloatImage image;
        image::readImage(path, image, image::EImageColorSpace::LINEAR);  // linear: sRGB conversion is done in display shader

        sourceSize = QSize(image.Width(), image.Height());

        // ensure it fits in GPU memory
        if (FloatTexture::maxTextureSize() != -1)
        {
            const auto maxTextureSize = FloatTexture::maxTextureSize();
            while (maxTextureSize != -1 &&
                (image.Width() > maxTextureSize || image.Height() > maxTextureSize))
            {
                FloatImage tmp;
                aliceVision::image::ImageHalfSample(image, tmp);
                image = std::move(tmp);
            }
        }

        // load metadata as well
        const auto metadata = image::readImageMetadata(path);
        for (const auto& item : metadata)
        {
            qmetadata[QString::fromStdString(item.name().string())] = QString::fromStdString(item.get_string());
        }

        // rotate image
        const auto orientation = metadata.get_int("orientation", 1);
        switch (orientation)
        {
        case 1:
            // do nothing
            break;
        case 3:
            rotate(image, RotateAngle::CW_180);
            break;
        case 6:
            rotate(image, RotateAngle::CW_90);
            break;
        case 8:
            rotate(image, RotateAngle::CW_270);
            break;
        default:
            qInfo() << "[QtAliceVision] Unsupported orientation: " << orientation << "\n";
        }

        result = QSharedPointer<FloatImage>(new FloatImage(std::move(image)));
    }
    catch (std::exception& e)
    {
        qInfo() << "[QtAliceVision] Failed to load image: " << _path
            << "\n" << e.what();
    }

    Q_EMIT resultReady(result, sourceSize, qmetadata);
}

FloatImageViewer::FloatImageViewer(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);
    connect(this, &FloatImageViewer::gammaChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::gainChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::textureSizeChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::sourceSizeChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::channelModeChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::imageChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::sourceChanged, this, &FloatImageViewer::reload);
    connect(this, &FloatImageViewer::verticesChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::gridColorChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::sfmChanged, this, &FloatImageViewer::update);
}

FloatImageViewer::~FloatImageViewer()
{
}

void FloatImageViewer::setLoading(bool loading)
{
    if (_loading == loading)
    {
        return;
    }
    _loading = loading;
    Q_EMIT loadingChanged();
}


QVector4D FloatImageViewer::pixelValueAt(int x, int y)
{
    if (!_image)
    {
        // qInfo() << "[QtAliceVision] FloatImageViewer::pixelValueAt(" << x << ", " << y << ") => no valid image";
        return QVector4D(0.0, 0.0, 0.0, 0.0);
    }
    else if (x < 0 || x >= _image->Width() ||
        y < 0 || y >= _image->Height())
    {
        // qInfo() << "[QtAliceVision] FloatImageViewer::pixelValueAt(" << x << ", " << y << ") => out of range";
        return QVector4D(0.0, 0.0, 0.0, 0.0);
    }
    aliceVision::image::RGBAfColor color = (*_image)(y, x);
    // qInfo() << "[QtAliceVision] FloatImageViewer::pixelValueAt(" << x << ", " << y << ") => valid pixel: " << color(0) << ", " << color(1) << ", " << color(2) << ", " << color(3);
    return QVector4D(color(0), color(1), color(2), color(3));
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

    if (!_source.isValid())
    {
        _image.reset();
        _imageChanged = true;
        Q_EMIT imageChanged();
        return;
    }

    if (!_loading)
    {
        setLoading(true);

        // async load from file
        auto ioRunnable = new FloatImageIORunnable(_source);
        connect(ioRunnable, &FloatImageIORunnable::resultReady, this, &FloatImageViewer::onResultReady);
        QThreadPool::globalInstance()->start(ioRunnable);
    }
    else
    {
        _outdated = true;
    }
}

void FloatImageViewer::onResultReady(QSharedPointer<FloatImage> image, QSize sourceSize, const QVariantMap& metadata)
{
    setLoading(false);

    if (_outdated)
    {
        // another request has been made while io thread was working
        _image.reset();
        reload();
        return;
    }

    _image = image;
    _imageChanged = true;
    Q_EMIT imageChanged();

    _sourceSize = sourceSize;
    Q_EMIT sourceSizeChanged();

    _metadata = metadata;
    Q_EMIT metadataChanged();
}

QSGNode* FloatImageViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
{
    QVector4D channelOrder(0.f, 1.f, 2.f, 3.f);

    QSGGeometryNode* root = static_cast<QSGGeometryNode*>(oldNode);
    QSGSimpleMaterial<ShaderData>* material = nullptr;

    QSGGeometry* geometryLine = nullptr;
    bool updateSfmData = false;

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
            auto material = new QSGFlatColorMaterial;
            material->setColor(_surface.gridColor());
            {
                // Vertexcount of the grid is equal to indexCount of the image
                geometryLine = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), _surface.indexCount());
                geometryLine->setDrawingMode(GL_LINES);
                geometryLine->setLineWidth(2);

                node->setGeometry(geometryLine);
                node->setFlags(QSGNode::OwnsGeometry);
                node->setMaterial(material);
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
        mat->setColor(_surface.gridColor());
        geometryLine = rootGrid->geometry();
    }

    if (_surface.hasSubsChanged())
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
    }

    bool updateGeometry = false;
    material->state()->gamma = _gamma;
    material->state()->gain = _gain;
    material->state()->channelOrder = channelOrder;

    if (_imageChanged)
    {
        if (_distortion || _surface.IsPanoViewerEnabled())
        {
            updateSfmData = true;
        }
            
        QSize newTextureSize;
        auto texture = std::make_unique<FloatTexture>();
        if (_image)
        {
            // Rotate Image if Disto Viewer enable
            if (_surface.IsPanoViewerEnabled())
            {
                qWarning() << "Before rotate";
                rotate(*_image, RotateAngle::CW_270);
                qWarning() << "After rotate";
            }

            texture->setImage(_image);
            texture->setFiltering(QSGTexture::Nearest);
            texture->setHorizontalWrapMode(QSGTexture::Repeat);
            texture->setVerticalWrapMode(QSGTexture::Repeat);
            newTextureSize = texture->textureSize();
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

        const float windowRatio = _boundingRect.width() / _boundingRect.height();
        const float textureRatio = _textureSize.width() / float(_textureSize.height());
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
    *   Surface
    */

    // If vertices has changed, Re-Compute the grid 
    if (_surface.hasVerticesChanged() && !_createRoot)
    {
        // Retrieve Vertices and Index Data
        QSGGeometry::TexturedPoint2D* vertices = root->geometry()->vertexDataAsTexturedPoint2D();
        quint16* indices = root->geometry()->indexDataAsUShort();

        // Load new sfm Data if there is distortion and sfm data has changed
        bool LoadSfm = _surface.loadSfmData(vertices, indices, _textureSize, _distortion, updateSfmData);

        root->geometry()->markIndexDataDirty();
        root->geometry()->markVertexDataDirty();
        root->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);

        // Fill the Surface vertices array
        _surface.fillVertices(vertices);
        Q_EMIT verticesChanged(true);

        if (LoadSfm)
        {
            _surface.verticesChanged(true);
            Q_EMIT sfmChanged();
        }
    }

    // Draw the grid if there Disto Viewer is enabled
    if (_distortion && _surface.hasGridChanged() && !_createRoot)
    {
        _surface.draw(geometryLine);
        Q_EMIT verticesChanged(false);
    }
    else if (!_distortion)
    {
        _surface.removeGrid(geometryLine);
    }

    root->childAtIndex(0)->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);

    return root;
}

/*
*   Q_INVOKABLE Functions 
*/

QPoint FloatImageViewer::getVertex(int index) 
{ 
    return _surface.vertex(index); 
}

void FloatImageViewer::setVertex(int index, float x, float y)
{
    QPoint point(x, y);
    _surface.vertex(index) = point;
    _surface.verticesChanged(true);
    _surface.gridChanged(true);
    Q_EMIT verticesChanged(false);
}

void FloatImageViewer::displayGrid(bool display)
{
    _surface.gridChanged(true);
    _surface.gridDisplayed(display);
    Q_EMIT verticesChanged(false);
}

void FloatImageViewer::setGridColorQML(const QColor& color)
{
    _surface.setGridColor(color);
    Q_EMIT gridColorChanged();
}

void FloatImageViewer::defaultControlPoints()
{
    _surface.clearVertices();
    _surface.reinitialize(true);
    _surface.verticesChanged(true);
    _surface.gridChanged(true);
    Q_EMIT verticesChanged(false);
}

void FloatImageViewer::resized()
{
    _surface.verticesChanged(true);
    _surface.gridChanged(true);
    Q_EMIT verticesChanged(false);
}

bool FloatImageViewer::reinit()
{
    return _surface.hasReinitialized();
}

void FloatImageViewer::hasDistortion(bool distortion)
{
    _distortion = distortion;
    _imageChanged = true;
    _surface.verticesChanged(true);
    _surface.clearVertices();
    Q_EMIT verticesChanged(false);

}

void FloatImageViewer::updateSubdivisions(int subs)
{
    _surface.subsChanged(true);
    _surface.setSubdivisions(subs);

    _surface.clearVertices();
    _surface.verticesChanged(true);
    _surface.gridChanged(true);
    Q_EMIT verticesChanged(false);
}

void FloatImageViewer::setSfmPath(const QString& path)
{
    _surface.setSfmPath(path);
    _imageChanged = true;
    _surface.verticesChanged(true);
    _surface.gridChanged(true);
    Q_EMIT verticesChanged(false);
}

QPoint FloatImageViewer::getPrincipalPoint()
{
    return _surface.principalPoint();
}

void FloatImageViewer::setIdView(int id)
{
    _surface.setIdView(id);
}

void FloatImageViewer::setPanoViewerEnabled(bool state)
{
    _surface.setPanoViewerState(state);
}


}  // qtAliceVision