#include "PhongImageViewer.hpp"

#include <QSGGeometry>
#include <QSGMaterial>
#include <QSGMaterialShader>
#include <QSGTexture>

namespace qtAliceVision {

namespace
{
    class PhongImageViewerMaterialShader;

    class PhongImageViewerMaterial : public QSGMaterial
    {
    public:
        PhongImageViewerMaterial() {}

        QSGMaterialType* type() const override
        {
            static QSGMaterialType type;
            return &type;
        }

        int compare(const QSGMaterial* other) const override
        {
            Q_ASSERT(other && type() == other->type());
            return other == this ? 0 : (other > this ? 1 : -1);
        }

        QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;

        struct
        {
            // warning: matches layout and padding of PhongImageViewer.vert/frag shaders
            QVector4D channelOrder = QVector4D(0, 1, 2, 3);
            QVector4D baseColor = QVector4D(.2f, .2f, .2f, 1.f);
            QVector3D lightDirection = QVector3D(0.f, 0.f, -1.f);
            float textureOpacity = 1.f;
            float gamma = 1.f;
            float gain = 0.f;
            float ka = 0.f;
            float kd = 1.f;
            float ks = 1.f;
            float shininess = 20.f;

        } uniforms;

        std::unique_ptr<FloatTexture> sourceTexture = std::make_unique<FloatTexture>(); // should be initialized
        std::unique_ptr<FloatTexture> normalTexture = std::make_unique<FloatTexture>(); // should be initialized
        bool dirtyUniforms;
    };

    class PhongImageViewerMaterialShader : public QSGMaterialShader
    {
    public:
        PhongImageViewerMaterialShader()
        {
            setShaderFileName(VertexStage, QLatin1String(":/shaders/PhongImageViewer.vert.qsb"));
            setShaderFileName(FragmentStage, QLatin1String(":/shaders/PhongImageViewer.frag.qsb"));
        }

        bool updateUniformData(RenderState& state, QSGMaterial* newMaterial, QSGMaterial* oldMaterial) override
        {
            bool changed = false;
            QByteArray* buf = state.uniformData();
            Q_ASSERT(buf->size() >= 84);
            if (state.isMatrixDirty())
            {
                const QMatrix4x4 m = state.combinedMatrix();
                memcpy(buf->data() + 0, m.constData(), 64);
                changed = true;
            }
            if (state.isOpacityDirty()) {
                const float opacity = state.opacity();
                memcpy(buf->data() + 64, &opacity, 4);
                changed = true;
            }
            auto* customMaterial = static_cast<PhongImageViewerMaterial*>(newMaterial);
            if (oldMaterial != newMaterial || customMaterial->dirtyUniforms) {
                memcpy(buf->data() + 80, &customMaterial->uniforms, 72); // uniforms size is 72
                customMaterial->dirtyUniforms = false;
                changed = true;
            }
            return changed;
        }

        void updateSampledImage(RenderState& state, int binding, QSGTexture** texture, QSGMaterial* newMaterial, QSGMaterial*) override
        {
            PhongImageViewerMaterial* mat = static_cast<PhongImageViewerMaterial*>(newMaterial);

            if (binding == 1)
            {
                mat->sourceTexture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
                *texture = mat->sourceTexture.get();
            }

            if (binding == 2)
            {
                mat->normalTexture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
                *texture = mat->normalTexture.get();
            }
        }
    };

    QSGMaterialShader* PhongImageViewerMaterial::createShader(QSGRendererInterface::RenderMode) const
    {
        return new PhongImageViewerMaterialShader;
    }

    class PhongImageViewerNode : public QSGGeometryNode
    {
    public:
        PhongImageViewerNode()
        {
            auto* m = new PhongImageViewerMaterial;
            setMaterial(m);
            setFlag(OwnsMaterial, true);
            QSGGeometry* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0);
            geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
            geometry->setVertexDataPattern(QSGGeometry::StaticPattern);
            setGeometry(geometry);
            setFlag(OwnsGeometry, true);
        }

        void setBlending(bool value)
        {
            auto* m = static_cast<PhongImageViewerMaterial*>(material());
            m->setFlag(QSGMaterial::Blending, value);
        }

        void setEmptyGeometry()
        {
            auto* g = geometry();

            if (g->vertexCount() != 0)
            {
                g->allocate(0);
                markDirty(QSGNode::DirtyGeometry);
            }
        }

        void setRectGeometry(const QRectF& bounds)
        {
            auto* g = geometry();

            if (g->vertexCount() != 4)
            {
                geometry()->allocate(4);
            }

            QSGGeometry::updateTexturedRectGeometry(g, bounds, QRectF(0, 0, 1, 1));

            markDirty(QSGNode::DirtyGeometry);
        }

        void setSourceParameters(const QVector4D& channelOrder, float gamma, float gain)
        {
            auto* m = static_cast<PhongImageViewerMaterial*>(material());
            m->uniforms.gamma = gamma;
            m->uniforms.gain = gain;
            m->uniforms.channelOrder = channelOrder;
            m->dirtyUniforms = true;
            markDirty(DirtyMaterial);
        }

        void setShadingParameters(const QVector4D& baseColor, const QVector3D& lightDirection, float textureOpacity, float ka, float kd, float ks, float shininess)
        {
            auto* m = static_cast<PhongImageViewerMaterial*>(material());
            m->uniforms.baseColor = baseColor;
            m->uniforms.lightDirection = lightDirection;
            m->uniforms.textureOpacity = textureOpacity;
            m->uniforms.ka = ka;
            m->uniforms.kd = kd;
            m->uniforms.ks = ks;
            m->uniforms.shininess = shininess;
            m->dirtyUniforms = true;
            markDirty(DirtyMaterial);
        }

        void setTextures(std::unique_ptr<FloatTexture> sourceTexture, std::unique_ptr<FloatTexture> normalTexture)
        {
            auto* m = static_cast<PhongImageViewerMaterial*>(material());
            m->sourceTexture = std::move(sourceTexture);
            m->normalTexture = std::move(normalTexture);
            markDirty(DirtyMaterial);
        }
    };
}

PhongImageViewer::PhongImageViewer(QQuickItem* parent)
  : QQuickItem(parent)
{
    // flags
    setFlag(QQuickItem::ItemHasContents, true);

    // connects
    connect(this, &PhongImageViewer::sourcePathChanged, this, &PhongImageViewer::reload);
    connect(this, &PhongImageViewer::normalPathChanged, this, &PhongImageViewer::reload);
    connect(this, &PhongImageViewer::sourceParametersChanged, this, [this] { _sourceParametersChanged = true; update(); });
    connect(this, &PhongImageViewer::shadingParametersChanged, this, [this] { _shadingParametersChanged = true; update(); });
    connect(this, &PhongImageViewer::textureSizeChanged, this, &PhongImageViewer::update);
    connect(this, &PhongImageViewer::sourceSizeChanged, this, &PhongImageViewer::update);
    connect(this, &PhongImageViewer::imageChanged, this, &PhongImageViewer::update);
    connect(&_sourceImageLoader, &imgserve::SingleImageLoader::requestHandled, this, &PhongImageViewer::reload);
    connect(&_normalImageLoader, &imgserve::SingleImageLoader::requestHandled, this, &PhongImageViewer::reload);
}

PhongImageViewer::~PhongImageViewer() {}

void PhongImageViewer::setStatus(EStatus status)
{
    if (_status == status) { return; }
    _status = status;
    Q_EMIT statusChanged();
}

void PhongImageViewer::reload()
{
    // check source image path and normal image path
    if (!_sourcePath.isValid() || !_normalPath.isValid())
    {
        clearImages();
        setStatus(EStatus::MISSING_FILE);
        return;
    }

    // use image server to request input images
    imgserve::RequestData requestSourceImage = {_sourcePath.toLocalFile().toUtf8().toStdString()};
    imgserve::RequestData requestNormalImage = {_normalPath.toLocalFile().toUtf8().toStdString()};
    imgserve::ResponseData responseSourceImage = _sourceImageLoader.request(requestSourceImage);
    imgserve::ResponseData responseNormalImage = _normalImageLoader.request(requestNormalImage);

    // check for loading errors
    if ((responseSourceImage.img == nullptr) || (responseNormalImage.img == nullptr))
    {
        if (responseSourceImage.error != imgserve::LoadingStatus::SUCCESSFUL)
        {
            clearImages();
            setStatus((responseSourceImage.error == imgserve::LoadingStatus::MISSING_FILE) ? EStatus::MISSING_FILE : EStatus::LOADING_ERROR);
        }
        else if (responseNormalImage.error != imgserve::LoadingStatus::SUCCESSFUL)
        {
            clearImages();
            setStatus((responseNormalImage.error == imgserve::LoadingStatus::MISSING_FILE) ? EStatus::MISSING_FILE : EStatus::LOADING_ERROR);
        }
        else
        {
            setStatus(EStatus::LOADING);
        }
        return;
    }

    // check source and normal images dimensions
    if (responseSourceImage.dim != responseSourceImage.dim)
    {
        clearImages();
        setStatus(EStatus::LOADING_ERROR);
        return;
    }

    // loading done
    setStatus(EStatus::NONE);

    // copy source image
    _sourceImage = responseSourceImage.img;
    
    // copy normal image
    _normalImage = responseNormalImage.img;
    
    // images have changed
    _imageChanged = true;
    Q_EMIT imageChanged();

    // source image dimensions have changed
    _sourceSize = responseSourceImage.dim;
    Q_EMIT sourceSizeChanged();

    // source image metadata have changed
    _metadata = responseSourceImage.metadata;
    Q_EMIT metadataChanged();
}

void PhongImageViewer::clearImages()
{
    _sourceImage.reset();
    _normalImage.reset();
    _imageChanged = true;
    Q_EMIT imageChanged();
}

QSGNode* PhongImageViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
{
    auto* node = static_cast<PhongImageViewerNode*>(oldNode);
    bool isNewNode = false;

    if (!node)
    {
        node = new PhongImageViewerNode();
        isNewNode = true;
    }

    // update textures
    if (_imageChanged)
    {
        QSize newTextureSize;
        auto sourceTexture = std::make_unique<FloatTexture>();
        auto normalTexture = std::make_unique<FloatTexture>();

        if(_sourceImage)
        {
            sourceTexture->setImage(_sourceImage);
            sourceTexture->setFiltering(QSGTexture::Nearest);
            sourceTexture->setHorizontalWrapMode(QSGTexture::Repeat);
            sourceTexture->setVerticalWrapMode(QSGTexture::Repeat);
            newTextureSize = sourceTexture->textureSize();
        }

        if (_normalImage)
        {
            normalTexture->setImage(_normalImage);
            normalTexture->setFiltering(QSGTexture::Nearest);
            normalTexture->setHorizontalWrapMode(QSGTexture::Repeat);
            normalTexture->setVerticalWrapMode(QSGTexture::Repeat);
        }

        node->setTextures(std::move(sourceTexture), std::move(normalTexture));

        if (_textureSize != newTextureSize)
        {
            _textureSize = newTextureSize;
            _geometryChanged = true;
            Q_EMIT textureSizeChanged();
        }

        _imageChanged = false;
    }

    const auto newBoundingRect = boundingRect();

    // update geometry
    if (_geometryChanged || _boundingRect != newBoundingRect)
    {
        _boundingRect = newBoundingRect;

        if (_textureSize.width() <= 0 || _textureSize.height() <= 0)
        {
            node->setEmptyGeometry();
        }
        else
        {
            const float windowRatio = _boundingRect.width() / _boundingRect.height();
            const float textureRatio = _textureSize.width() / float(_textureSize.height());
            QRectF geometryRect = _boundingRect;

            if (windowRatio > textureRatio)
                geometryRect.setWidth(geometryRect.height() * textureRatio);
            else
                geometryRect.setHeight(geometryRect.width() / textureRatio);

            geometryRect.moveCenter(_boundingRect.center());

            static const int MARGIN = 0;
            geometryRect = geometryRect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);

            node->setRectGeometry(geometryRect);
        }

        _geometryChanged = false;
    }

    // update shader source parameters
    if (isNewNode || _sourceParametersChanged)
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
        }

        node->setSourceParameters(channelOrder, _gamma, _gain);
        node->setBlending(_channelMode == EChannelMode::RGBA);

        _sourceParametersChanged = false;
    }

    // update shader Blinn-Phong parameters
    if (isNewNode || _shadingParametersChanged)
    {
        // light direction from Yaw and Pitch
        const Eigen::AngleAxis<double> yawA(static_cast<double>(aliceVision::degreeToRadian(_lightYaw)), Eigen::Vector3d::UnitY());
        const Eigen::AngleAxis<double> pitchA(static_cast<double>(aliceVision::degreeToRadian(_lightPitch)), Eigen::Vector3d::UnitX());
        const aliceVision::Vec3 direction = yawA.toRotationMatrix() * pitchA.toRotationMatrix() * aliceVision::Vec3(0.0, 0.0, -1.0);
        const QVector3D lightDirection(direction.x(), direction.y(), direction.z());

        // linear base color from QColor
        const QVector4D baseColor(std::powf(_baseColor.redF(), 2.2), std::powf(_baseColor.greenF(), 2.2), std::powf(_baseColor.blueF(), 2.2), _baseColor.alphaF());

        node->setShadingParameters(baseColor, lightDirection, _textureOpacity, _ka, _kd, _ks, _shininess);

        _shadingParametersChanged = false;
    }

    return node;
}

}  // namespace qtAliceVision
