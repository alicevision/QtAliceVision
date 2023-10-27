#include "PointCloudEntity.hpp"

#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DCore/QTransform>

namespace sfmdataentity
{

PointCloudEntity::PointCloudEntity(Qt3DCore::QNode* parent)
    : Qt3DCore::QEntity(parent)
{
}

void PointCloudEntity::setData(const aliceVision::sfmData::Landmarks & landmarks)
{
    using namespace Qt3DRender;

    // create a new geometry renderer
    auto customMeshRenderer = new QGeometryRenderer;
    auto customGeometry = new QGeometry;

    std::vector<float> points;
    std::vector<float> colors;
    for (const auto & l : landmarks)
    {
        points.push_back(static_cast<float>(l.second.X(0)));
        points.push_back(static_cast<float>(- l.second.X(1)));
        points.push_back(static_cast<float>(- l.second.X(2)));

        colors.push_back(static_cast<float>(l.second.rgb(0) / 255.0f));
        colors.push_back(static_cast<float>(l.second.rgb(1) / 255.0f));
        colors.push_back(static_cast<float>(l.second.rgb(2) / 255.0f));
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
