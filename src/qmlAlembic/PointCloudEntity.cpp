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
    size_t npoints = positions->size();

    // vertices buffer
    QByteArray positionData((const char*)positions->get(), npoints * 3 * sizeof(float));
    auto vertexDataBuffer = new QBuffer(QBuffer::VertexBuffer);
    vertexDataBuffer->setData(positionData);
    auto positionAttribute = new QAttribute;
    positionAttribute->setAttributeType(QAttribute::VertexAttribute);
    positionAttribute->setBuffer(vertexDataBuffer);
    positionAttribute->setDataType(QAttribute::Float);
    positionAttribute->setDataSize(3);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(3 * sizeof(float));
    positionAttribute->setCount(npoints);
    positionAttribute->setName(QAttribute::defaultPositionAttributeName());
    customGeometry->addAttribute(positionAttribute);
    customGeometry->setBoundingVolumePositionAttribute(positionAttribute);

    // read color data
    auto colorDataBuffer = new QBuffer(QBuffer::VertexBuffer);

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
                    Alembic::AbcCoreAbstract::DataType dType = prop.getDataType();
                    Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                    prop.get(samp);
                    QByteArray colorData((const char*)samp->getData(),
                                         samp->size() * 3 * sizeof(float));
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
        for(int i = 0; i < positions->size() * 3; i++)
            colors[i] = 0.8f;
        QByteArray colorData((const char*)colors, npoints * 3 * sizeof(float));
        colorDataBuffer->setData(colorData);
    }

    // colors buffer
    auto colorAttribute = new QAttribute;
    colorAttribute->setAttributeType(QAttribute::VertexAttribute);
    colorAttribute->setBuffer(colorDataBuffer);
    colorAttribute->setDataType(QAttribute::Float);
    colorAttribute->setDataSize(3);
    colorAttribute->setByteOffset(0);
    colorAttribute->setByteStride(3 * sizeof(float));
    colorAttribute->setCount(npoints);
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
