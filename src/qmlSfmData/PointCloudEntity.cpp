#include "PointCloudEntity.hpp"

#include <QGeometryRenderer>
#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QBuffer>
#include <Qt3DCore/QTransform>

namespace sfmdataentity {

PointCloudEntity::PointCloudEntity(Qt3DCore::QNode* parent)
  : Qt3DCore::QEntity(parent)
{}

void PointCloudEntity::setData(const aliceVision::sfmData::Landmarks& landmarks)
{
    using namespace Qt3DRender;
    using namespace Qt3DCore;

    // create a new geometry renderer
    auto customMeshRenderer = new QGeometryRenderer;
    auto customGeometry = new QGeometry;

    std::vector<float> points;
    std::vector<float> colors;
    for (const auto& l : landmarks)
    {
      const aliceVision::Vec3 & pt = l.second.getX();
      const aliceVision::image::RGBColor & color = l.second.getRgb();
    
      points.push_back(static_cast<float>(pt.x()));
      points.push_back(static_cast<float>(-pt.y()));
      points.push_back(static_cast<float>(-pt.z()));

      colors.push_back(static_cast<float>(color.r() / 255.0f));
      colors.push_back(static_cast<float>(color.g() / 255.0f));
      colors.push_back(static_cast<float>(color.b() / 255.0f));
    }

    int npoints = static_cast<int>(landmarks.size());

    // vertices buffer
    QByteArray positionData(reinterpret_cast<const char*>(points.data()), npoints * 3 * static_cast<int>(sizeof(float)));
    auto vertexDataBuffer = new QBuffer;
    vertexDataBuffer->setData(positionData);
    auto positionAttribute = new QAttribute;
    positionAttribute->setAttributeType(QAttribute::VertexAttribute);
    positionAttribute->setBuffer(vertexDataBuffer);
    positionAttribute->setVertexBaseType(QAttribute::Float);
    positionAttribute->setVertexSize(3);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(3 * sizeof(float));
    positionAttribute->setCount(static_cast<uint>(npoints));
    positionAttribute->setName(QAttribute::defaultPositionAttributeName());
    customGeometry->addAttribute(positionAttribute);
    customGeometry->setBoundingVolumePositionAttribute(positionAttribute);

    // read color data
    auto colorDataBuffer = new QBuffer;
    QByteArray colorData(reinterpret_cast<const char*>(colors.data()), npoints * 3 * static_cast<int>(sizeof(float)));
    colorDataBuffer->setData(colorData);

    // colors buffer
    auto colorAttribute = new QAttribute;
    colorAttribute->setAttributeType(QAttribute::VertexAttribute);
    colorAttribute->setBuffer(colorDataBuffer);
    colorAttribute->setVertexBaseType(QAttribute::Float);
    colorAttribute->setVertexSize(3);
    colorAttribute->setByteOffset(0);
    colorAttribute->setByteStride(3 * sizeof(float));
    colorAttribute->setCount(static_cast<uint>(npoints));
    colorAttribute->setName(QAttribute::defaultColorAttributeName());
    customGeometry->addAttribute(colorAttribute);

    // normals buffer
    // The per-vertex-color / diffuse-specular materials used for the SfM display declare a
    // vertexNormal input. On the macOS Metal RHI backend a geometry that omits that attribute
    // makes pipeline creation fail and crashes the renderer (e.g. on viewer resize). Point
    // clouds have no meaningful surface normal, so provide a constant placeholder (0, 1, 0)
    // purely to satisfy the vertex descriptor.
    std::vector<float> normals(static_cast<size_t>(npoints) * 3, 0.0f);
    for (int i = 0; i < npoints; ++i)
        normals[static_cast<size_t>(i) * 3 + 1] = 1.0f;
    auto normalDataBuffer = new QBuffer(customGeometry);
    QByteArray normalData(reinterpret_cast<const char*>(normals.data()), npoints * 3 * static_cast<int>(sizeof(float)));
    normalDataBuffer->setData(normalData);
    auto normalAttribute = new QAttribute(customGeometry);
    normalAttribute->setAttributeType(QAttribute::VertexAttribute);
    normalAttribute->setBuffer(normalDataBuffer);
    normalAttribute->setVertexBaseType(QAttribute::Float);
    normalAttribute->setVertexSize(3);
    normalAttribute->setByteOffset(0);
    normalAttribute->setByteStride(3 * sizeof(float));
    normalAttribute->setCount(static_cast<uint>(npoints));
    normalAttribute->setName(QAttribute::defaultNormalAttributeName());
    customGeometry->addAttribute(normalAttribute);

    // geometry renderer settings
    customMeshRenderer->setInstanceCount(1);
    customMeshRenderer->setFirstVertex(0);
    customMeshRenderer->setFirstInstance(0);
    customMeshRenderer->setPrimitiveType(QGeometryRenderer::Points);
    customMeshRenderer->setGeometry(customGeometry);
    customMeshRenderer->setVertexCount(npoints);

    // add components
    addComponent(customMeshRenderer);
}

}  // namespace sfmdataentity
