#include "CameraLocatorEntity.hpp"

#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QObjectPicker>

#include <aliceVision/numeric/numeric.hpp>

#include <boost/math/constants/constants.hpp>

namespace sfmdataentity {

CameraLocatorEntity::CameraLocatorEntity(const aliceVision::IndexT& viewId, const aliceVision::IndexT& resectionId,
                                         float hfov, float vfov, Qt3DCore::QNode* parent)
  : Qt3DCore::QEntity(parent),
    _viewId(viewId),
    _resectionId(resectionId)
{
    _transform = new Qt3DCore::QTransform;
    addComponent(_transform);

    using namespace Qt3DRender;

    // create a new geometry renderer
    auto customMeshRenderer = new QGeometryRenderer;
    auto customGeometry = new QGeometry;

    const float axisLength = 0.5f;
    const float halfImageWidth = 0.3f;
    const float halfImageHeight = 0.2f;
    const float yArrowHeight = 0.05f;
    const float depth = halfImageWidth / tan(hfov / 2.0);
    const float radius = 0.3f;

    int subdiv = 1;
    if (hfov > boost::math::constants::pi<double>() * 0.7 || vfov > boost::math::constants::pi<double>() * 0.7)
        subdiv = 10;

    // clang-format off
    // vertices buffer
    QVector<float> origin = {
        // Coord system
        0.f, 0.f, 0.f, axisLength, 0.0f, 0.0f,  // X
        0.f, 0.f, 0.f, 0.0f, -axisLength, 0.0f,  // Y
        0.f, 0.f, 0.f, 0.0f, 0.0f, -axisLength,  // Z
    };

    QVector<float> bodyCam;

    aliceVision::Vec3 tl, tr, bl, br;
    tl.x() = sin(-hfov / 2.0f);
    tl.y() = sin(vfov / 2.0f);
    tl.z() = cos(-hfov / 2.0f);

    tr.x() = sin(-hfov / 2.0f);
    tr.y() = sin(vfov / 2.0f);
    tr.z() = cos(vfov / 2.0f);

    const float vslice = vfov / static_cast<double>(subdiv);
    const float hslice = hfov / static_cast<double>(subdiv);

    Eigen::Vector3d vZ = - Eigen::Vector3d::UnitZ() * radius;

    for (int vid = 0; vid < subdiv; vid++)
    {
        float vangle1 = - vfov / 2.0f + (vid) * vslice;
        float vangle2 = - vfov / 2.0f + (vid + 1) * vslice;

        Eigen::AngleAxis<double> Rv1(vangle1, Eigen::Vector3d::UnitX());
        Eigen::AngleAxis<double> Rv2(vangle2, Eigen::Vector3d::UnitX());

        for (int hid = 0; hid < subdiv; hid++)
        {
            float hangle1 = - hfov / 2.0f + (hid) * hslice;
            float hangle2 = - hfov / 2.0f + (hid + 1) * hslice;

            Eigen::AngleAxis<double> Rh1(hangle1, Eigen::Vector3d::UnitY());
            Eigen::AngleAxis<double> Rh2(hangle2, Eigen::Vector3d::UnitY());

            aliceVision::Vec3f pt1 = (Rv1.toRotationMatrix() * Rh1.toRotationMatrix() * vZ).cast<float>();
            aliceVision::Vec3f pt2 = (Rv2.toRotationMatrix() * Rh1.toRotationMatrix() * vZ).cast<float>();
            aliceVision::Vec3f pt3 = (Rv2.toRotationMatrix() * Rh2.toRotationMatrix() * vZ).cast<float>();
            aliceVision::Vec3f pt4 = (Rv1.toRotationMatrix() * Rh2.toRotationMatrix() * vZ).cast<float>();

            QVector<float> quad = {
                pt1.x(), pt1.y(), pt1.z(), pt2.x(), pt2.y(), pt2.z(),
                pt2.x(), pt2.y(), pt2.z(), pt3.x(), pt3.y(), pt3.z(),
                pt3.x(), pt3.y(), pt3.z(), pt4.x(), pt4.y(), pt4.z(),
                pt4.x(), pt4.y(), pt4.z(), pt1.x(), pt1.y(), pt1.z(),
            };

            bodyCam.append(quad);
        }
    }

    float vangle1 = - vfov / 2.0f;
    float vangle2 = vfov / 2.0f;
    float hangle1 = - hfov / 2.0f;
    float hangle2 = hfov / 2.0f;

    Eigen::AngleAxis<double> Rv1(vangle1, Eigen::Vector3d::UnitX());
    Eigen::AngleAxis<double> Rv2(vangle2, Eigen::Vector3d::UnitX());
    Eigen::AngleAxis<double> Rh1(hangle1, Eigen::Vector3d::UnitY());
    Eigen::AngleAxis<double> Rh2(hangle2, Eigen::Vector3d::UnitY());

    aliceVision::Vec3f pt1 = (Rv1.toRotationMatrix() * Rh1.toRotationMatrix() * vZ).cast<float>();
    aliceVision::Vec3 pt2 = Rv2.toRotationMatrix() * Rh1.toRotationMatrix() * vZ;
    auto pt2f = pt2.cast<float>();
    aliceVision::Vec3 pt3 = Rv2.toRotationMatrix() * Rh2.toRotationMatrix() * vZ;
    auto pt3f = pt3.cast<float>();
    aliceVision::Vec3f pt4 = (Rv1.toRotationMatrix() * Rh2.toRotationMatrix() * vZ).cast<float>();

    QVector<float> quad = {
        0.0f, 0.0f, 0.0f, pt2f.x(), pt2f.y(), pt2f.z(),
        0.0f, 0.0f, 0.0f, pt3f.x(), pt3f.y(), pt3f.z(),
        0.0f, 0.0f, 0.0f, pt4.x(), pt4.y(), pt4.z(),
        0.0f, 0.0f, 0.0f, pt1.x(), pt1.y(), pt1.z(),
    };

    bodyCam.append(quad);

    aliceVision::Vec3f middle = (0.5 * (pt2 + pt3)).cast<float>();

    QVector<float> upVector = {
        // Camera Up
        pt2f.x(), pt2f.y(), pt2f.z(), middle.x(), middle.y() + yArrowHeight, middle.z(),
        middle.x(), middle.y() + yArrowHeight, middle.z(), pt3f.x(), pt3f.y(), pt3f.z()
    };

    QVector<float> points;
    points.append(origin);
    points.append(bodyCam);
    points.append(upVector);
    // clang-format on

    QByteArray positionData(reinterpret_cast<const char*>(points.data()), points.size() * static_cast<int>(sizeof(float)));
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
    _colors = initializeColors(points.size(), 1.0f);

    QByteArray colorData(reinterpret_cast<const char*>(_colors.data()), _colors.size() * static_cast<int>(sizeof(float)));
    auto colorDataBuffer = new QBuffer;
    colorDataBuffer->setData(colorData);
    _colorAttribute = new QAttribute;
    _colorAttribute->setAttributeType(QAttribute::VertexAttribute);
    _colorAttribute->setBuffer(colorDataBuffer);
    _colorAttribute->setVertexBaseType(QAttribute::Float);
    _colorAttribute->setVertexSize(3);
    _colorAttribute->setByteOffset(0);
    _colorAttribute->setByteStride(3 * sizeof(float));
    _colorAttribute->setCount(static_cast<uint>(_colors.size() / 3));
    _colorAttribute->setName(QAttribute::defaultColorAttributeName());
    customGeometry->addAttribute(_colorAttribute);

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

QVector<float> CameraLocatorEntity::initializeColors(int size, float defaultValue)
{
    QVector<float> colors(size, defaultValue);
    float* color;

    // R
    color = &(colors[0 * 3]);
    color[0] = 1.0f;
    color[1] = 0.0f;
    color[2] = 0.0f;

    // R
    color = &(colors[1 * 3]);
    color[0] = 1.0f;
    color[1] = 0.0f;
    color[2] = 0.0f;

    // G
    color = &(colors[2 * 3]);
    color[0] = 0.0f;
    color[1] = 1.0f;
    color[2] = 0.0f;

    // G
    color = &(colors[3 * 3]);
    color[0] = 0.0f;
    color[1] = 1.0f;
    color[2] = 0.0f;

    // B
    color = &(colors[4 * 3]);
    color[0] = 0.0f;
    color[1] = 0.0f;
    color[2] = 1.0f;

    // B
    color = &(colors[5 * 3]);
    color[0] = 0.0f;
    color[1] = 0.0f;
    color[2] = 1.0f;

    return colors;
}

void CameraLocatorEntity::updateColors(float red, float green, float blue)
{
    const int pyramidIndex = 6 * 3;  // Only modify the colors of the pyramid, image plane and camera up direction
    for (int i = pyramidIndex; i < _colors.size(); i += 3)
    {
        _colors[i] = red;
        _colors[i + 1] = green;
        _colors[i + 2] = blue;
    }

    QByteArray colorData(reinterpret_cast<const char*>(_colors.data()), _colors.size() * static_cast<int>(sizeof(float)));
    auto colorDataBuffer = new Qt3DRender::QBuffer;
    colorDataBuffer->setData(colorData);
    _colorAttribute->setBuffer(colorDataBuffer);
}

}  // namespace sfmdataentity
