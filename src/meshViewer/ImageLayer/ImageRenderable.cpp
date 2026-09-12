#include <ImageLayer/ImageRenderable.hpp>
#include <ImageLayer/ImageLayer.hpp>
#include <Geometry/vertex.hpp>
#include <QFile>

namespace {

// Fullscreen quad in NDC: two triangles covering [-1..1] x [-1..1].
constexpr QuadVertex k_quadVertices[] = {
  {-1.0f, -1.0f},  //  0: bottom-left
  {1.0f, -1.0f},   //  1: bottom-right
  {-1.0f, 1.0f},   //  2: top-left
  {1.0f, 1.0f},    //  3: top-right
};

constexpr quint16 k_quadIndices[] = {0, 1, 2, 1, 3, 2};

QShader loadShader(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        qFatal("Cannot open shader %s", qPrintable(path));
    }

    return QShader::fromSerialized(f.readAll());
}

}  // namespace

void ImageRenderable::sync(LayerItem* layer, SceneView* view)
{
    ImageLayer* imageLayer = static_cast<ImageLayer*>(layer);
    if (imageLayer->imageDirty())
    {
        _image = imageLayer->takeImage();
        _textureDirty = true;
    }
}

void ImageRenderable::initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc)
{
    if (_rhi != rhi)
    {
        _rhi = rhi;
        _rpDesc = rpDesc;
        _pipelineDirty = true;
        _textureDirty = (_image.size() > 0);
        _pipeline.reset();
        _srb.reset();
        _sampler.reset();
        _texture.reset();
        _uniformBuffer.reset();
        _vertexBuffer.reset();
        _indexBuffer.reset();
    }

    // Defer pipeline creation until the first image is available so that the
    // texture can be sized correctly from the start.
    if (_pipelineDirty && _image.size() > 0)
    {
        buildPipeline();
    }
}

void ImageRenderable::prepare(QRhiResourceUpdateBatch* batch, const SceneState& state)
{
    // Image arrives asynchronously after initialize() — build the pipeline on the
    // first frame where both RHI context and image data are available.
    if (_pipelineDirty && _image.size() > 0)
    {
        buildPipeline();
    }

    if (!_pipeline)
    {
        return;
    }

    if (_geomDirty)
    {
        buildGeometry(batch);
    }

    if (_textureDirty && _image.size() > 0)
    {
        uploadTexture(batch);
    }

    if (_uniformBuffer && _image.size() > 0)
    {
        QMatrix4x4 imageProjection = state.orthogonalMatrix;
        batch->updateDynamicBuffer(_uniformBuffer.get(), 0, 64, imageProjection.constData());
    }
}

void ImageRenderable::render(QRhiCommandBuffer* cb, const SceneState& state)
{
    if (!_pipeline || !_vertexBuffer || !_texture)
    {
        return;
    }

    cb->setGraphicsPipeline(_pipeline.get());
    cb->setViewport({0, 0, static_cast<float>(state.viewportWidth), static_cast<float>(state.viewportHeight)});
    cb->setShaderResources(_srb.get());

    const QRhiCommandBuffer::VertexInput vb(_vertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &vb, _indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(static_cast<quint32>(std::size(k_quadIndices)));
}

void ImageRenderable::buildPipeline()
{
    _pipeline.reset();
    _srb.reset();
    _sampler.reset();
    _texture.reset();
    _uniformBuffer.reset();
    _vertexBuffer.reset();
    _indexBuffer.reset();

    // Uniform buffer: mat4 imageProjection (64 bytes, std140).
    _uniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    _uniformBuffer->create();

    // Sampler: bilinear, clamp-to-edge, no mipmaps.
    _sampler.reset(_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    _sampler->create();

    // Texture sized for the current image; pixel data is uploaded in uploadTexture().
    _texture.reset(_rhi->newTexture(QRhiTexture::RGBA32F, QSize(_image.width(), _image.height())));
    _texture->create();

    // SRB: binding 0 = UBO (vertex), binding 1 = combined sampler (fragment).
    _srb.reset(_rhi->newShaderResourceBindings());
    _srb->setBindings({
      QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, _uniformBuffer.get()),
      QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, _texture.get(), _sampler.get()),
    });
    _srb->create();

    const QShader vert = loadShader(QStringLiteral(":/shaders/imageQuad.vert.qsb"));
    const QShader frag = loadShader(QStringLiteral(":/shaders/imageQuad.frag.qsb"));

    // Vertex layout: vec2 position only.
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding(sizeof(QuadVertex))});
    inputLayout.setAttributes({
      QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0),
    });

    _pipeline.reset(_rhi->newGraphicsPipeline());
    _pipeline->setShaderStages({
      {QRhiShaderStage::Vertex, vert},
      {QRhiShaderStage::Fragment, frag},
    });
    _pipeline->setVertexInputLayout(inputLayout);
    _pipeline->setShaderResourceBindings(_srb.get());
    _pipeline->setRenderPassDescriptor(_rpDesc);
    _pipeline->setDepthTest(false);
    _pipeline->setDepthWrite(false);
    _pipeline->setCullMode(QRhiGraphicsPipeline::None);
    _pipeline->create();

    _pipelineDirty = false;
    _geomDirty = true;
    _textureDirty = true;
}

void ImageRenderable::buildGeometry(QRhiResourceUpdateBatch* batch)
{
    _vertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, static_cast<quint32>(sizeof(k_quadVertices))));
    _vertexBuffer->create();

    _indexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, static_cast<quint32>(sizeof(k_quadIndices))));
    _indexBuffer->create();

    batch->uploadStaticBuffer(_vertexBuffer.get(), k_quadVertices);
    batch->uploadStaticBuffer(_indexBuffer.get(), k_quadIndices);

    _geomDirty = false;
}

void ImageRenderable::uploadTexture(QRhiResourceUpdateBatch* batch)
{
    _textureDirty = false;

    const int w = _image.width();
    const int h = _image.height();
    const QSize imageSize(w, h);

    // Recreate the texture (and rebuild the SRB) only when the image dimensions change.
    if (!_texture || _texture->pixelSize() != imageSize)
    {
        _texture.reset(_rhi->newTexture(QRhiTexture::RGBA32F, imageSize));
        _texture->create();

        _srb.reset(_rhi->newShaderResourceBindings());
        _srb->setBindings({
          QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, _uniformBuffer.get()),
          QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, _texture.get(), _sampler.get()),
        });
        _srb->create();
    }

    // RGBAfColor is 4 contiguous floats per pixel in row-major order, matching
    // RGBA32F exactly. _image is a member so data() remains valid until
    // cb->resourceUpdate(batch) consumes the batch in the same frame.
    const quint32 byteCount = static_cast<quint32>(w * h * 4 * sizeof(float));
    batch->uploadTexture(
      _texture.get(), QRhiTextureUploadDescription(QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(_image.data(), byteCount))));
}
