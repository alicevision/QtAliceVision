#include "PointCloudEntity.hpp"
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DCore/QTransform>

namespace abcentity
{

PointCloudEntity::PointCloudEntity(Qt3DCore::QNode* parent)
    : BaseAlembicObject(parent)
{
}

void PointCloudEntity::setData(const Alembic::Abc::IObject& iObj)
{
    using namespace Qt3DRender;
    using namespace Alembic::Abc;
    using namespace Alembic::AbcGeom;

    // create a new geometry renderer
    auto customMeshRenderer = new QGeometryRenderer;
    auto customGeometry = new QGeometry;

    // read position data
    IPoints points(iObj, kWrapExisting);
    IPointsSchema schema = points.getSchema();
    P3fArraySamplePtr positions = schema.getValue().getPositions();
    int npoints = static_cast<int>(positions->size());

    // vertices buffer
    QByteArray positionData((const char*)positions->get(), npoints * 3 * static_cast<int>(sizeof(float)));
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

    // check if we have a color property
    ICompoundProperty cProp = schema.getArbGeomParams();
    if(cProp)
    {
        std::size_t numProps = cProp.getNumProperties();
        for(std::size_t i = 0; i < numProps; ++i)
        {
            const PropertyHeader& propHeader = cProp.getPropertyHeader(i);
            if(propHeader.isArray())
            {
                const std::string& propName = propHeader.getName();
                Alembic::Abc::IArrayProperty prop(cProp, propName);
                std::string interp = prop.getMetaData().get("interpretation");
                if(interp == "rgb")
                {
                    // Alembic::AbcCoreAbstract::DataType dType = prop.getDataType();
                    Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                    prop.get(samp);
                    QByteArray colorData((const char*)samp->getData(),
                                         static_cast<int>(samp->size() * 3 * sizeof(float)));
                    colorDataBuffer->setData(colorData);
                    break; // set colors only once
                }
            }
        }
    }

    // if needed, fill the buffer with a default color
    if(colorDataBuffer->data().isEmpty())
    {
        auto colors = new float[positions->size() * 3];
        for(size_t i = 0; i < positions->size() * 3; i++)
            colors[i] = 0.8f;
        QByteArray colorData((const char*)colors, npoints * 3 * static_cast<int>(sizeof(float)));
        colorDataBuffer->setData(colorData);
    }

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


} // namespace
