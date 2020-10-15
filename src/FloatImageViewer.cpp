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

namespace
{
    enum class RotateAngle
    {
        CW_90,
        CW_180,
        CW_270,
    };

    void copyPixelRotate90(FloatImage & dst, const FloatImage & src, int x, int y)
    {
        dst(x, src.Height() - y - 1) = src(y, x);
    }

    void copyPixelRotate270(FloatImage & dst, const FloatImage & src, int x, int y)
    {
        dst(src.Width() - x - 1, y) = src(y, x);
    }

    void copyPixelRotate180(FloatImage & dst, const FloatImage & src, int x, int y)
    {
        dst(src.Height() - y - 1, src.Width() - x - 1) = src(y, x);
    }

    template <class T>
    void forEach(FloatImage & dstImage, const FloatImage & srcImage, T && f)
    {
        for(int y=0; y!=srcImage.Height(); ++y)
        {
            for(int x=0; x!=srcImage.Width(); ++x)
            {
                f(dstImage, srcImage, x, y);
            }
        }
    }

    void rotate(FloatImage & image, RotateAngle angle)
    {
        switch(angle)
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

FloatImageIORunnable::FloatImageIORunnable(const QUrl & path, QObject * parent)
    : QObject(parent)
    , _path(path)
{}

void FloatImageIORunnable::run()
{
    using namespace aliceVision;
    QSharedPointer<FloatImage> result;
    QSize sourceSize(0,0);
    QVariantMap qmetadata;

    try 
    {
        const auto path = _path.toLocalFile().toUtf8().toStdString();
        FloatImage image;
        image::readImage(path, image, image::EImageColorSpace::LINEAR);  // linear: sRGB conversion is done in display shader

        sourceSize = QSize(image.Width(), image.Height());

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

        // load metadata as well
        const auto metadata = image::readImageMetadata(path);
        for(const auto & item : metadata)
        {
            qmetadata[QString::fromStdString(item.name().string())] = QString::fromStdString(item.get_string());
        }

        // rotate image
        const auto orientation = metadata.get_int("orientation", 1);
        switch(orientation)
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
    catch(std::exception& e)
    {
        qInfo() << "[QtAliceVision] Failed to load image: " << _path
                 << "\n" << e.what();
    }

    Q_EMIT resultReady(result, sourceSize, qmetadata);
}

namespace
{
struct ShaderData
{
    float gamma = 1.f;
    float gain = 0.f;
    QVector4D channelOrder = QVector4D(0,1,2,3);
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
        "uniform lowp float qt_Opacity;                                                  \n"
        "uniform highp sampler2D texture;                                                \n"
        "uniform lowp float gamma;                                                       \n"
        "uniform lowp float gain;                                                        \n"
        "uniform vec4 channelOrder;                                                      \n"
        "varying highp vec2 vTexCoord;                                                   \n"
        "void main() {                                                                   \n"
        "    vec4 color = texture2D(texture, vTexCoord);                                 \n"
        "    color.rgb = pow(pow((color.rgb * vec3(gain)), vec3(1.0/gamma)), vec3(1.0 / 2.2)); \n"
        "    gl_FragColor.r = color[int(channelOrder[0])];                               \n"
        "    gl_FragColor.g = color[int(channelOrder[1])];                               \n"
        "    gl_FragColor.b = color[int(channelOrder[2])];                               \n"
        "    gl_FragColor.a = int(channelOrder[3]) == -1 ? 1.0 : color[int(channelOrder[3])]; \n"
        "    gl_FragColor.a *= qt_Opacity; \n"
        "    gl_FragColor.rgb *= gl_FragColor.a; \n"
        "}";
    }

    QList<QByteArray> attributes() const override
    {
        return QList<QByteArray>() << "vertex" << "texCoord";
    }

    void updateState(const ShaderData *data, const ShaderData *) override
    {
        program()->setUniformValue(_gammaId, data->gamma);
        program()->setUniformValue(_gainId, data->gain);
        program()->setUniformValue(_channelOrder, data->channelOrder);

        if(data->texture)
        {
            data->texture->bind();
        }
    }

    void resolveUniforms() override
    {
        _textureId = program()->uniformLocation("texture");
        _gammaId = program()->uniformLocation("gamma");
        _gainId = program()->uniformLocation("gain");
        _channelOrder = program()->uniformLocation("channelOrder");

        // We will only use texture unit 0, so set it only once.
        program()->setUniformValue(_textureId, 0);
    }

private:
    int _textureId = -1;
    int _gammaId = -1;
    int _gainId = -1;
    int _channelOrder = -1;
};
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
    if(!_image)
    {
        // qInfo() << "[QtAliceVision] FloatImageViewer::pixelValueAt(" << x << ", " << y << ") => no valid image";
        return QVector4D(0.0, 0.0, 0.0, 0.0);
    }
    else if(x < 0 || x >= _image->Width() ||
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

void FloatImageViewer::onResultReady(QSharedPointer<FloatImage> image, QSize sourceSize, const QVariantMap & metadata)
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

    _sourceSize = sourceSize;
    Q_EMIT sourceSizeChanged();

    _metadata = metadata;
    Q_EMIT metadataChanged();
}

QSGNode* FloatImageViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
{
    QVector4D channelOrder(0.f,1.f,2.f,3.f);

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

    // enable Blending flag for transparency for RGBA
    material->setFlag(QSGMaterial::Blending, _channelMode == EChannelMode::RGBA); 

    // change channelOrder according to channelMode
    switch(_channelMode)
    {
        case EChannelMode::R :
            channelOrder = QVector4D(0.f,0.f,0.f,-1.f);
            break;
        case EChannelMode::G :
            channelOrder = QVector4D(1.f,1.f,1.f,-1.f);
            break;
        case EChannelMode::B :
            channelOrder = QVector4D(2.f,2.f,2.f,-1.f);
            break;
        case EChannelMode::A :
            channelOrder = QVector4D(3.f,3.f,3.f,-1.f);
            break;
    }

    bool updateGeometry = false;
    material->state()->gamma = _gamma;
    material->state()->gain = _gain;
    material->state()->channelOrder = channelOrder;

    if(_imageChanged)
    {
        QSize newTextureSize;
        auto texture = std::make_unique<FloatTexture>();
        if(_image)
        {
            texture->setImage(_image);
            texture->setFiltering(QSGTexture::Nearest);
            texture->setHorizontalWrapMode(QSGTexture::Repeat);
            texture->setVerticalWrapMode(QSGTexture::Repeat);
            newTextureSize = texture->textureSize();
        }
        material->state()->texture = std::move(texture);

        _imageChanged = false;

        if(_textureSize != newTextureSize)
        {
            _textureSize = newTextureSize;
            updateGeometry = true;
            Q_EMIT textureSizeChanged();
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
