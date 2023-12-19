#include "DepthMapEntity.hpp"

#include <QDebug>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QShaderProgram>
#include <Qt3DRender/QTechnique>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/io.hpp>
#include <aliceVision/mvsData/Matrix3x3.hpp>
#include <aliceVision/mvsData/Point2d.hpp>
#include <aliceVision/mvsData/Point3d.hpp>
#include <aliceVision/image/jetColorMap.hpp>
#include <aliceVision/numeric/numeric.hpp>

#include <cmath>
#include <iostream>

using namespace aliceVision;

namespace depthMapEntity {

DepthMapEntity::DepthMapEntity(Qt3DCore::QNode* parent)
  : Qt3DCore::QEntity(parent),
    _displayMode(DisplayMode::Unknown),
    _pointSizeParameter(new Qt3DRender::QParameter)
{
    qDebug() << "[DepthMapEntity] DepthMapEntity";
    createMaterials();
}

void DepthMapEntity::setSource(const QUrl& value)
{
    if (_source == value)
        return;

    if (!value.isValid())
    {
        qDebug() << "[DepthMapEntity] Invalid source: " << value;
        _status = DepthMapEntity::Error;
        return;
    }

    _source = value;

    QFileInfo fileInfo = QFileInfo(_source.path());
    QString filename = fileInfo.fileName();
    if (filename.contains("depthMap"))
    {
        _depthMapSource = _source;
        _simMapSource = QUrl::fromLocalFile(QFileInfo(fileInfo.dir(), filename.replace("depthMap", "simMap")).filePath());
    }
    else if (filename.contains("simMap"))
    {
        _simMapSource = _source;
        _depthMapSource = QUrl::fromLocalFile(QFileInfo(fileInfo.dir(), filename.replace("simMap", "depthMap")).filePath());
    }
    else
    {
        qWarning() << "[DepthMapEntity] Source filename must contain depthMap or simMap: " << filename;
        _status = DepthMapEntity::Error;
        return;
    }

    loadDepthMap();
    Q_EMIT sourceChanged();
}

void DepthMapEntity::setDisplayMode(const DepthMapEntity::DisplayMode& value)
{
    if (_displayMode == value)
        return;
    _displayMode = value;
    updateMaterial();

    Q_EMIT displayModeChanged();
}

void DepthMapEntity::setDisplayColor(bool value)
{
    if (_displayColor == value)
        return;
    _displayColor = value;
    updateMaterial();

    Q_EMIT displayColorChanged();
}

void DepthMapEntity::updateMaterial()
{
    if (_status != DepthMapEntity::Ready)
        return;

    Qt3DRender::QMaterial* newMaterial = nullptr;

    switch (_displayMode)
    {
        case DisplayMode::Points:
            newMaterial = _cloudMaterial;
            _meshRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Points);
            break;
        case DisplayMode::Triangles:
            _meshRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
            if (_displayColor)
                newMaterial = _colorMaterial;
            else
                newMaterial = _diffuseMaterial;
            break;
        default:
            newMaterial = _diffuseMaterial;
    }

    if (newMaterial == _currentMaterial)
        return;

    if (_currentMaterial)
        removeComponent(_currentMaterial);

    _currentMaterial = newMaterial;
    addComponent(_currentMaterial);
}

void DepthMapEntity::setPointSize(const float& value)
{
    if (_pointSize == value)
        return;
    _pointSize = value;
    _pointSizeParameter->setValue(value);
    _cloudMaterial->setEnabled(_pointSize > 0.0f);
    Q_EMIT pointSizeChanged();
}

// private
void DepthMapEntity::createMaterials()
{
    using namespace Qt3DRender;
    using namespace Qt3DExtras;

    {
        _cloudMaterial = new QMaterial(this);

        // configure cloud material
        QEffect* effect = new QEffect;
        QTechnique* technique = new QTechnique;
        QRenderPass* renderPass = new QRenderPass;
        QShaderProgram* shaderProgram = new QShaderProgram;

        // set vertex shader
        shaderProgram->setVertexShaderCode(R"(#version 130
        in vec3 vertexPosition;
        in vec3 vertexColor;
        out vec3 color;
        uniform mat4 mvp;
        uniform mat4 projectionMatrix;
        uniform mat4 viewportMatrix;
        uniform float pointSize;
        void main()
        {
            color = vertexColor;
            gl_Position = mvp * vec4(vertexPosition, 1.0);
            gl_PointSize = max(viewportMatrix[1][1] * projectionMatrix[1][1] * pointSize / gl_Position.w, 1.0);
        }
        )");

        // set fragment shader
        shaderProgram->setFragmentShaderCode(R"(#version 130
            in vec3 color;
            out vec4 fragColor;
            void main(void)
            {
                fragColor = vec4(color, 1.0);
            }
        )");

        // add a pointSize uniform
        _pointSizeParameter->setName("pointSize");
        _pointSizeParameter->setValue(_pointSize);
        _cloudMaterial->addParameter(_pointSizeParameter);

        // build the material
        renderPass->setShaderProgram(shaderProgram);
        technique->addRenderPass(renderPass);
        effect->addTechnique(technique);
        _cloudMaterial->setEffect(effect);
    }
    {
        _colorMaterial = new QPerVertexColorMaterial(this);
    }
    {
        _diffuseMaterial = new QDiffuseSpecularMaterial(this);
        _diffuseMaterial->setAmbient(QColor(0, 0, 0));
        _diffuseMaterial->setDiffuse(QColor(255, 255, 255));
        _diffuseMaterial->setSpecular(QColor(0, 0, 0));
        _diffuseMaterial->setShininess(0.0);
    }
}

bool validTriangleRatio(const Vec3f& a, const Vec3f& b, const Vec3f& c)
{
    std::vector<double> distances = {DistanceL2(a, b), DistanceL2(b, c), DistanceL2(c, a)};
    double mi = std::min({distances[0], distances[1], distances[2]});
    double ma = std::max({distances[0], distances[1], distances[2]});
    if (ma == 0.0)
        return false;
    return (mi / ma) > 1.0 / 5.0;
}

// private
void DepthMapEntity::loadDepthMap()
{
    using namespace Qt3DRender;

    _status = DepthMapEntity::Loading;

    if (_meshRenderer)
    {
        removeComponent(_meshRenderer);
        _meshRenderer->deleteLater();
        _meshRenderer = nullptr;
    }

    // Load depth map and metadata

    const std::string depthMapPath = _depthMapSource.toLocalFile().toStdString();
    qDebug() << "[DepthMapEntity] Load depth map: " << _depthMapSource.toLocalFile();
    image::Image<float> depthMap;
    try
    {
        image::readImage(depthMapPath, depthMap, image::EImageColorSpace::LINEAR);
    }
    catch (const std::runtime_error& error)
    {
        qCritical() << "[DepthMapEntity] Could not load depth map:" << error.what();
        _status = DepthMapEntity::Error;
        return;
    }

    oiio::ImageBuf inBuf;
    image::getBufferFromImage(depthMap, inBuf);

    qDebug() << "[DepthMapEntity] Image Size: " << depthMap.Width() << "x" << depthMap.Height();

    oiio::ImageSpec inSpec = image::readImageSpec(depthMapPath);

    Point3d CArr;
    oiio::ParamValue* cParam = inSpec.find_attribute("AliceVision:CArr");
    if (!cParam)
    {
        qWarning() << "[DepthMapEntity] Missing metadata CArr.";
        _status = DepthMapEntity::Error;
        return;
    }

    qDebug() << "[DepthMapEntity] CArr: "
             << " nvalues: " << cParam->nvalues() << ", type: " << cParam->type().c_str() << ", basetype: " << cParam->type().basetype
             << ", aggregate: " << cParam->type().aggregate << ", vecsemantics: " << cParam->type().vecsemantics
             << ", arraylen: " << cParam->type().arraylen;

    if (cParam->type().aggregate != oiio::TypeDesc::AGGREGATE::VEC3)
    {
        qWarning() << "[DepthMapEntity] Metadata CArr: Type error (aggregate: " << cParam->type().aggregate << ")";
        _status = DepthMapEntity::Error;
        return;
    }
    if (cParam->type().basetype == oiio::TypeDesc::BASETYPE::DOUBLE)
    {
        std::copy_n(static_cast<const double*>(cParam->data()), 3, CArr.m);
    }
    else if (cParam->type().basetype == oiio::TypeDesc::BASETYPE::FLOAT)
    {
        const float* d = static_cast<const float*>(cParam->data());
        for (int i = 0; i < 3; ++i)
        {
            CArr.m[i] = d[i];
        }
    }
    else
    {
        qWarning() << "[DepthMapEntity] Metadata CArr: Data type error (basetype: " << cParam->type().basetype << ")";
        _status = DepthMapEntity::Error;
        return;
    }

    Matrix3x3 iCamArr;
    oiio::ParamValue* icParam = inSpec.find_attribute("AliceVision:iCamArr");

    if (!icParam)
    {
        qWarning() << "[DepthMapEntity] Missing metadata iCamArr.";
        _status = DepthMapEntity::Error;
        return;
    }

    qDebug() << "[DepthMapEntity] iCamArr: "
             << " nvalues: " << icParam->nvalues() << ", type: " << icParam->type().c_str() << ", basetype: " << icParam->type().basetype
             << ", aggregate: " << icParam->type().aggregate << ", vecsemantics: " << icParam->type().vecsemantics
             << ", arraylen: " << icParam->type().arraylen;
    if (icParam->type().aggregate != oiio::TypeDesc::AGGREGATE::MATRIX33)
    {
        qWarning() << "[DepthMapEntity] Metadata iCamArr: Type error (aggregate: " << icParam->type().aggregate << ")";
        _status = DepthMapEntity::Error;
        return;
    }
    if (icParam->type().basetype == oiio::TypeDesc::BASETYPE::DOUBLE)
    {
        std::copy_n(static_cast<const double*>(icParam->data()), 9, iCamArr.m);
    }
    else if (icParam->type().basetype == oiio::TypeDesc::BASETYPE::FLOAT)
    {
        const float* d = static_cast<const float*>(icParam->data());
        for (int i = 0; i < 9; ++i)
        {
            iCamArr.m[i] = d[i];
        }
    }
    else
    {
        qWarning() << "[DepthMapEntity] Metadata iCamArr: Data type error (basetype: " << icParam->type().basetype << ")";
        _status = DepthMapEntity::Error;
        return;
    }

    // Load sim map

    image::Image<float> simMap;
    if (_simMapSource.isValid())
    {
        const std::string simMapPath = _simMapSource.toLocalFile().toStdString();
        qDebug() << "[DepthMapEntity] Load sim map: " << _simMapSource.toLocalFile();
        try
        {
            image::readImage(simMapPath, simMap, image::EImageColorSpace::LINEAR);
        }
        catch (const std::runtime_error& error)
        {
            qWarning() << "[DepthMapEntity] Sim map could not be loaded:" << error.what();
        }
    }
    else
    {
        qWarning() << "[DepthMapEntity] Failed to find associated sim map";
    }

    const bool validSimMap = (simMap.Width() == depthMap.Width()) && (simMap.Height() == depthMap.Height());

    // 3D points position and color (using jetColorMap)

    qDebug() << "[DepthMapEntity] Computing positions and colors for point cloud";

    std::vector<int> indexPerPixel(static_cast<std::size_t>(depthMap.Width() * depthMap.Height()), -1);
    std::vector<Vec3f> positions;
    std::vector<image::RGBfColor> colors;

    oiio::ImageBufAlgo::PixelStats stats = oiio::ImageBufAlgo::computePixelStats(inBuf);

    for (int y = 0; y < depthMap.Height(); ++y)
    {
        for (int x = 0; x < depthMap.Width(); ++x)
        {
            float depthValue = depthMap(y, x);
            if (!std::isfinite(depthValue) || depthValue <= 0.f)
                continue;

            Point3d p = CArr + (iCamArr * Point2d(static_cast<double>(x), static_cast<double>(y))).normalize() * depthValue;
            Vec3f position(static_cast<float>(p.x), static_cast<float>(-p.y), static_cast<float>(-p.z));

            indexPerPixel[static_cast<std::size_t>(y * depthMap.Width() + x)] = static_cast<int>(positions.size());
            positions.push_back(position);

            if (validSimMap)
            {
                float simValue = simMap(y, x);
                image::RGBfColor color = getColorFromJetColorMap(simValue);
                colors.push_back(color);
            }
            else
            {
                const float range = stats.max[0] - stats.min[0];
                float normalizedDepthValue = range != 0.0f ? (depthValue - stats.min[0]) / range : 1.0f;
                image::RGBfColor color = getColorFromJetColorMap(normalizedDepthValue);
                colors.push_back(color);
            }
        }
    }

    // Create geometry

    qDebug() << "[DepthMapEntity] Creating geometry";

    QGeometry* customGeometry = new QGeometry;

    // vertices buffer
    std::vector<std::size_t> trianglesIndexes;
    trianglesIndexes.reserve(2 * 3 * positions.size());
    for (int y = 0; y < depthMap.Height() - 1; ++y)
    {
        for (int x = 0; x < depthMap.Width() - 1; ++x)
        {
            int pixelIndexA = indexPerPixel[static_cast<std::size_t>(y * depthMap.Width() + x)];
            int pixelIndexB = indexPerPixel[static_cast<std::size_t>((y + 1) * depthMap.Width() + x)];
            int pixelIndexC = indexPerPixel[static_cast<std::size_t>((y + 1) * depthMap.Width() + x + 1)];
            int pixelIndexD = indexPerPixel[static_cast<std::size_t>(y * depthMap.Width() + x + 1)];

            // Cast indices to std::size_t once for readability
            std::size_t sPixelIndexA = static_cast<std::size_t>(pixelIndexA);
            std::size_t sPixelIndexB = static_cast<std::size_t>(pixelIndexB);
            std::size_t sPixelIndexC = static_cast<std::size_t>(pixelIndexC);
            std::size_t sPixelIndexD = static_cast<std::size_t>(pixelIndexD);

            if (pixelIndexA != -1 && pixelIndexB != -1 && pixelIndexC != -1 &&
                validTriangleRatio(positions[sPixelIndexA], positions[sPixelIndexB], positions[sPixelIndexC]))
            {
                trianglesIndexes.push_back(sPixelIndexA);
                trianglesIndexes.push_back(sPixelIndexB);
                trianglesIndexes.push_back(sPixelIndexC);
            }
            if (pixelIndexC != -1 && pixelIndexD != -1 && pixelIndexA != -1 &&
                validTriangleRatio(positions[sPixelIndexC], positions[sPixelIndexD], positions[sPixelIndexA]))
            {
                trianglesIndexes.push_back(sPixelIndexC);
                trianglesIndexes.push_back(sPixelIndexD);
                trianglesIndexes.push_back(sPixelIndexA);
            }
        }
    }
    qDebug() << "[DepthMapEntity] Nb triangles: " << trianglesIndexes.size();

    std::vector<Vec3f> triangles;
    triangles.resize(trianglesIndexes.size());
    for (std::size_t i = 0; i < trianglesIndexes.size(); ++i)
    {
        triangles[i] = positions[trianglesIndexes[i]];
    }
    std::vector<Vec3f> normals;
    normals.resize(triangles.size());
    for (std::size_t i = 0; i < trianglesIndexes.size(); i += 3)
    {
        Vec3f normal = (triangles[i + 1] - triangles[i]).cross(triangles[i + 2] - triangles[i]);
        for (std::size_t t = 0; t < 3; ++t)
            normals[i + t] = normal;
    }

    QBuffer* vertexBuffer = new QBuffer;
    QByteArray trianglesData(reinterpret_cast<const char*>(&triangles[0]), static_cast<int>(triangles.size() * sizeof(Vec3f)));
    vertexBuffer->setData(trianglesData);

    QBuffer* normalBuffer = new QBuffer;
    QByteArray normalsData(reinterpret_cast<const char*>(&normals[0]), static_cast<int>(normals.size() * sizeof(Vec3f)));
    normalBuffer->setData(normalsData);

    QAttribute* positionAttribute = new QAttribute(this);
    positionAttribute->setName(QAttribute::defaultPositionAttributeName());
    positionAttribute->setAttributeType(QAttribute::VertexAttribute);
    positionAttribute->setBuffer(vertexBuffer);
    positionAttribute->setVertexBaseType(QAttribute::Float);
    positionAttribute->setVertexSize(3);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(sizeof(Vec3f));
    positionAttribute->setCount(static_cast<uint>(triangles.size()));

    QAttribute* normalAttribute = new QAttribute(this);
    normalAttribute->setName(QAttribute::defaultNormalAttributeName());
    normalAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    normalAttribute->setBuffer(normalBuffer);
    normalAttribute->setVertexBaseType(QAttribute::Float);
    normalAttribute->setVertexSize(3);
    normalAttribute->setByteOffset(0);
    normalAttribute->setByteStride(sizeof(Vec3f));
    normalAttribute->setCount(static_cast<uint>(normals.size()));

    customGeometry->addAttribute(positionAttribute);
    customGeometry->addAttribute(normalAttribute);
    // customGeometry->setBoundingVolumePositionAttribute(positionAttribute);

    // Duplicate colors as we cannot use indexes!
    std::vector<image::RGBfColor> colorsFlat;
    colorsFlat.reserve(trianglesIndexes.size());
    for (std::size_t i = 0; i < trianglesIndexes.size(); ++i)
    {
        colorsFlat.push_back(colors[trianglesIndexes[i]]);
    }

    // read color data
    QBuffer* colorDataBuffer = new QBuffer;
    QByteArray colorData(reinterpret_cast<const char*>(colorsFlat[0].data()), static_cast<int>(colorsFlat.size() * 3 * sizeof(float)));
    colorDataBuffer->setData(colorData);

    QAttribute* colorAttribute = new QAttribute;
    qDebug() << "[DepthMapEntity] Qt3DRender::QAttribute::defaultColorAttributeName(): " << Qt3DRender::QAttribute::defaultColorAttributeName();
    colorAttribute->setName(Qt3DRender::QAttribute::defaultColorAttributeName());
    colorAttribute->setAttributeType(QAttribute::VertexAttribute);
    colorAttribute->setBuffer(colorDataBuffer);
    colorAttribute->setVertexBaseType(QAttribute::Float);
    colorAttribute->setVertexSize(3);
    colorAttribute->setByteOffset(0);
    colorAttribute->setByteStride(3 * sizeof(float));
    colorAttribute->setCount(static_cast<uint>(colorsFlat.size()));
    customGeometry->addAttribute(colorAttribute);

    // create the geometry renderer
    _meshRenderer = new QGeometryRenderer;
    _meshRenderer->setGeometry(customGeometry);

    _status = DepthMapEntity::Ready;

    // add components
    addComponent(_meshRenderer);
    updateMaterial();
    qDebug() << "[DepthMapEntity] Mesh Renderer added";
}

}  // namespace depthMapEntity
