#include "FloatImageViewer.hpp"
#include "FloatTexture.hpp"

#include <QSGGeometryNode>
#include <QSGGeometry>
#include <QSGSimpleMaterial>
#include <QSGSimpleMaterialShader>
#include <QSGTexture>
#include <QThreadPool>

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/resampling.hpp>
#include <aliceVision/image/io.hpp>

namespace qtAliceVision 
{

FloatImageIORunnable::FloatImageIORunnable(const QUrl & path, QObject * parent)
    : QObject(parent)
    , _path(path)
{}

void FloatImageIORunnable::run()
{
    using namespace aliceVision;
    QSharedPointer<FloatImage> result;
    QVariantMap qmetadata;

    try 
    {
        const auto path = _path.toLocalFile().toUtf8().toStdString();
        FloatImage image;
        image::readImage(path, image, image::EImageColorSpace::SRGB);

        // ensure it fits in GPU memory
        if(FloatTexture::maxTextureSize() != -1)
        {
            const auto maxTextureSize = FloatTexture::maxTextureSize();
            while(maxTextureSize != -1 &&
                (image.Width() > maxTextureSize || image.Height() > maxTextureSize))
            {
                FloatImage tmp;
                aliceVision::image::ImageHalfSample(image, tmp);
                image = std::move(tmp);
            }
        }
        result = QSharedPointer<FloatImage>(new FloatImage(std::move(image)));
        
        // load metadata as well
        const auto metadata = image::readImageMetadata(path);
        for(const auto & item : metadata)
        {
            qmetadata[QString::fromStdString(item.name().string())] = QString::fromStdString(item.get_string());
        }
    }
    catch(std::exception& e)
    {
        qDebug() << "[QtAliceVision] Failed to load image: " << _path
                 << "\n" << e.what();
    }

    Q_EMIT resultReady(result, qmetadata);
}

namespace
{
struct ShaderData
{
    float gamma = 1.f;
    float offset = 1.f;
    std::unique_ptr<QSGTexture> texture;
};

class ImageViewerShader : public QSGSimpleMaterialShader<ShaderData>
{
    QSG_DECLARE_SIMPLE_SHADER(ImageViewerShader, ShaderData)

public:
    const char *vertexShader() const override
    {
        return
        "attribute highp vec4 vertex;              \n"
        "attribute highp vec2 texCoord;            \n"
        "uniform highp mat4 qt_Matrix;              \n"
        "varying highp vec2 vTexCoord;              \n"
        "void main() {                              \n"
        "    gl_Position = qt_Matrix * vertex;      \n"
        "    vTexCoord = texCoord;      \n"
        "}";
    }

    const char *fragmentShader() const override
    {
        return
        "uniform lowp float qt_Opacity;             \n"
        "uniform sampler2D texture;                 \n"
        "uniform lowp float gamma;                  \n"
        "uniform lowp float offset;                 \n"
        "varying highp vec2 vTexCoord;              \n"
        "void main() {                              \n"
        "    vec4 color = texture2D(texture, vTexCoord);         \n"
        "    gl_FragColor = vec4(color.rg * gamma, offset, color.a*qt_Opacity);     \n"
        "}";
    }

    QList<QByteArray> attributes() const override
    {
        return QList<QByteArray>() << "vertex" << "texCoord";
    }

    void updateState(const ShaderData *data, const ShaderData *) override
    {
        program()->setUniformValue(_gammaId, data->gamma);
        program()->setUniformValue(_offsetId, data->offset);

        if(data->texture)
        {
            data->texture->bind();
        }
    }

    void resolveUniforms() override
    {
        _textureId = program()->uniformLocation("texture");
        _gammaId = program()->uniformLocation("gamma");
        _offsetId = program()->uniformLocation("offset");

        // We will only use texture unit 0, so set it only once.
        program()->setUniformValue(_textureId, 0);
    }

private:
    int _textureId = -1;
    int _gammaId = -1;
    int _offsetId = -1;
};
}

FloatImageViewer::FloatImageViewer(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);
    connect(this, &FloatImageViewer::gammaChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::offsetChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::imageChanged, this, &FloatImageViewer::update);
    connect(this, &FloatImageViewer::sourceChanged, this, &FloatImageViewer::reload);
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

void FloatImageViewer::reload()
{
    if(_clearBeforeLoad)
    {
        _image.reset();
        _imageChanged = true;
        Q_EMIT imageChanged();
    }
    _outdated = false;

    if(!_source.isValid())
    {
        _image.reset();
        _imageChanged = true;
        Q_EMIT imageChanged();
        return;
    }

    if(!_loading)
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

void FloatImageViewer::onResultReady(QSharedPointer<FloatImage> image, const QVariantMap & metadata)
{
    setLoading(false);

    if(_outdated)
    {
        // another request has been made while io thread was working
        _image.reset();
        reload();
        return;
    }

    _image = image;
    _imageChanged = true;
    Q_EMIT imageChanged();

    _metadata = metadata;
    Q_EMIT metadataChanged();
}

QSGNode* FloatImageViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
{
    QSGGeometryNode* root = static_cast<QSGGeometryNode*>(oldNode);
    QSGSimpleMaterial<ShaderData>* material = nullptr;
    if(!root)
    {
        root = new QSGGeometryNode;

        auto geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
        geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
        geometry->setVertexDataPattern(QSGGeometry::StaticPattern);
        root->setGeometry(geometry);
        root->setFlags(QSGNode::OwnsGeometry);

        material = ImageViewerShader::createMaterial();
        root->setMaterial(material);
        root->setFlags(QSGNode::OwnsMaterial);
    }
    else
    {
        material = static_cast<QSGSimpleMaterial<ShaderData>*>(root->material());
    }

    bool updateGeometry = false;
    material->state()->gamma = _gamma;
    material->state()->offset = _offset;
    if(_imageChanged)
    {
        QSize newTextureSize;
        auto texture = std::make_unique<FloatTexture>();
        if(_image)
        {
            texture->setImage(std::move(*_image));
            texture->setFiltering(QSGTexture::Nearest);
            texture->setHorizontalWrapMode(QSGTexture::Repeat);
            texture->setVerticalWrapMode(QSGTexture::Repeat);
            newTextureSize = texture->textureSize();
        }
        material->state()->texture = std::move(texture);

        _image.reset();
        _imageChanged = false;

        if(_textureSize != newTextureSize)
        {
            _textureSize = newTextureSize;
            updateGeometry = true;
        }
    }

    const auto newBoundingRect = boundingRect();
    if(updateGeometry || _boundingRect != newBoundingRect)
    {
        _boundingRect = newBoundingRect;

        const float windowRatio = _boundingRect.width() / _boundingRect.height();
        const float textureRatio = _textureSize.width() / float(_textureSize.height());
        QRectF geometryRect = _boundingRect;
        if(windowRatio > textureRatio)
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
    return root;
}

}  // qtAliceVision
