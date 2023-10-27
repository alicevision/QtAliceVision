#include "CameraLocatorEntity.hpp"

#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QObjectPicker>
#include <Qt3DCore/QTransform>

namespace sfmdataentity {

CameraLocatorEntity::CameraLocatorEntity(const aliceVision::IndexT& viewId, Qt3DCore::QNode* parent)
  : Qt3DCore::QEntity(parent),
    _viewId(viewId)
{
    _transform = new Qt3DCore::QTransform;
    addComponent(_transform);

    using namespace Qt3DRender;

    // create a new geometry renderer
    auto customMeshRenderer = new QGeometryRenderer;
    auto customGeometry = new QGeometry;

    // vertices buffer
    QVector<float> points = {
      // Coord system
      0.f,
      0.f,
      0.f,
      0.5f,
      0.0f,
      0.0f,  // X
      0.f,
      0.f,
      0.f,
      0.0f,
      -0.5f,
      0.0f,  // Y
      0.f,
      0.f,
      0.f,
      0.0f,
      0.0f,
      -0.5f,  // Z

      // Pyramid
      0.f,
      0.f,
      0.f,
      -0.3f,
      0.2f,
      -0.3f,  // TL
      0.f,
      0.f,
      0.f,
      -0.3f,
      -0.2f,
      -0.3f,  // BL
      0.f,
      0.f,
      0.f,
      0.3f,
      -0.2f,
      -0.3f,  // BR
      0.f,
      0.f,
      0.f,
      0.3f,
      0.2f,
      -0.3f,  // TR

      // Image plane
      -0.3f,
      -0.2f,
      -0.3f,
      -0.3f,
      0.2f,
      -0.3f,  // L
      -0.3f,
      0.2f,
      -0.3f,
      0.3f,
      0.2f,
      -0.3f,  // B
      0.3f,
      0.2f,
      -0.3f,
      0.3f,
      -0.2f,
      -0.3f,  // R
      0.3f,
      -0.2f,
      -0.3f,
      -0.3f,
      -0.2f,
      -0.3f,  // T

      // Camera Up
      -0.3f,
      0.2f,
      -0.3f,
      0.0f,
      0.25f,
      -0.3f,  // L
      0.3f,
      0.2f,
      -0.3f,
      0.0f,
      0.25f,
      -0.3f,  // R
    };

    QByteArray positionData((const char*)points.data(), points.size() * static_cast<int>(sizeof(float)));
    auto vertexDataBuffer = new QBuffer;
    vertexDataBuffer->setData(positionData);
    auto positionAttribute = new QAttribute;
    positionAttribute->setAttributeType(QAttribute::VertexAttribute);
    positionAttribute->setBuffer(vertexDataBuffer);
    positionAttribute->setVertexBaseType(QAttribute::Float);
    positionAttribute->setVertexSize(3);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(3 * sizeof(float));
    positionAttribute->setCount(static_cast<uint>(points.size() / 3));
    positionAttribute->setName(QAttribute::defaultPositionAttributeName());
    customGeometry->addAttribute(positionAttribute);

    // colors buffer
    QVector<float> colors{
      // Coord system
      1.f,
      0.f,
      0.f,
      1.f,
      0.f,
      0.f,  // R
      0.f,
      1.f,
      0.f,
      0.f,
      1.f,
      0.f,  // G
      0.f,
      0.f,
      1.f,
      0.f,
      0.f,
      1.f,  // B
      // Pyramid
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      // Image Plane
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      // Camera Up direction
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
      1.f,
    };

    QByteArray colorData((const char*)colors.data(), colors.size() * static_cast<int>(sizeof(float)));
    auto colorDataBuffer = new QBuffer;
    colorDataBuffer->setData(colorData);
    auto colorAttribute = new QAttribute;
    colorAttribute->setAttributeType(QAttribute::VertexAttribute);
    colorAttribute->setBuffer(colorDataBuffer);
    colorAttribute->setVertexBaseType(QAttribute::Float);
    colorAttribute->setVertexSize(3);
    colorAttribute->setByteOffset(0);
    colorAttribute->setByteStride(3 * sizeof(float));
    colorAttribute->setCount(static_cast<uint>(colors.size() / 3));
    colorAttribute->setName(QAttribute::defaultColorAttributeName());
    customGeometry->addAttribute(colorAttribute);

    // geometry renderer settings
    customMeshRenderer->setInstanceCount(1);
    customMeshRenderer->setFirstVertex(0);
    customMeshRenderer->setFirstInstance(0);
    customMeshRenderer->setPrimitiveType(QGeometryRenderer::Lines);
    customMeshRenderer->setGeometry(customGeometry);
    customMeshRenderer->setVertexCount(points.size() / 3);

    // add components
    addComponent(customMeshRenderer);
}

void CameraLocatorEntity::setTransform(const Eigen::Matrix4d& T)
{
    Eigen::Matrix4d M;
    M.setIdentity();
    M(1, 1) = -1;
    M(2, 2) = -1;

    Eigen::Matrix4d mat = (M * T * M).inverse();

    QMatrix4x4 qmat(static_cast<float>(mat(0, 0)),
                    static_cast<float>(mat(0, 1)),
                    static_cast<float>(mat(0, 2)),
                    static_cast<float>(mat(0, 3)),

                    static_cast<float>(mat(1, 0)),
                    static_cast<float>(mat(1, 1)),
                    static_cast<float>(mat(1, 2)),
                    static_cast<float>(mat(1, 3)),

                    static_cast<float>(mat(2, 0)),
                    static_cast<float>(mat(2, 1)),
                    static_cast<float>(mat(2, 2)),
                    static_cast<float>(mat(2, 3)),

                    static_cast<float>(mat(3, 0)),
                    static_cast<float>(mat(3, 1)),
                    static_cast<float>(mat(3, 2)),
                    static_cast<float>(mat(3, 3)));

    _transform->setMatrix(qmat);
}

}  // namespace sfmdataentity
