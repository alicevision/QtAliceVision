#include "BaseAlembicObject.hpp"

namespace abcentity
{

BaseAlembicObject::BaseAlembicObject(Qt3DCore::QNode* parent)
    : Qt3DCore::QEntity(parent)
{
    _transform = new Qt3DCore::QTransform;
    addComponent(_transform);
}

void BaseAlembicObject::fillArbProperties(const Alembic::Abc::ICompoundProperty &iParent)
{
    fillPropertyMap(iParent, _arbProperties);
}

void BaseAlembicObject::fillUserProperties(const Alembic::Abc::ICompoundProperty &iParent)
{
    fillPropertyMap(iParent, _userProperties);
}

void BaseAlembicObject::setTransform(const Alembic::Abc::M44d& mat)
{
    QMatrix4x4 qmat(mat[0][0], mat[1][0], mat[2][0], mat[3][0], mat[0][1], mat[1][1], mat[2][1],
                    mat[3][1], mat[0][2], mat[1][2], mat[2][2], mat[3][2], mat[0][3], mat[1][3],
                    mat[2][3], mat[3][3]);
    _transform->setMatrix(qmat);
}


template<typename PODTYPE>
void BaseAlembicObject::addScalarProperty(QVariantMap& data, const Alembic::Abc::IScalarProperty& prop)
{
    static const Alembic::Abc::ISampleSelector iss((Alembic::Abc::index_t)0);

    // TODO: handle extent and interpretation
    PODTYPE val;
    prop.get(&val, iss);
    data[prop.getName().c_str()] = val;
}

template<>
void BaseAlembicObject::addScalarProperty<std::string>(QVariantMap& data, const Alembic::Abc::IScalarProperty& prop)
{
    std::string val;
    static const Alembic::Abc::ISampleSelector iss((Alembic::Abc::index_t)0);
    prop.get(&val, iss);
    data[prop.getName().c_str()] = QString::fromStdString(val);
}


template<typename PODTYPE>
void BaseAlembicObject::addArrayProperty(QVariantMap& data, const Alembic::Abc::IArrayProperty& prop)
{
    Alembic::AbcCoreAbstract::ArraySamplePtr val;
    prop.get(val);
    const PODTYPE* _data = static_cast<const PODTYPE*>(val->getData());
    QVariantList l;
    l.reserve(static_cast<int>(val->size()));
    for(size_t k=0; k < val->size(); k++)
    {
        l.append(_data[k]);
    }
    data[prop.getName().c_str()] = l;
}

template<>
void BaseAlembicObject::addArrayProperty<std::string>(QVariantMap& data, const Alembic::Abc::IArrayProperty& prop)
{
    Alembic::AbcCoreAbstract::ArraySamplePtr val;
    prop.get(val);
    const std::string* _data = static_cast<const std::string*>(val->getData());
    QVariantList l;
    l.reserve(static_cast<int>(val->size()));
    for(size_t k=0; k < val->size(); k++)
    {
        l.append(QString::fromStdString(_data[k]));
    }
    data[prop.getName().c_str()] = l;
}

template<typename PODTYPE>
void BaseAlembicObject::addProperty(QVariantMap& data, const Alembic::Abc::ICompoundProperty& iParent,
                                    const Alembic::Abc::PropertyHeader& propHeader)
{
    if(propHeader.isArray())
    {
        Alembic::Abc::IArrayProperty prop(iParent, propHeader.getName());
        if(!prop.isConstant())
            return;
        addArrayProperty<PODTYPE>(data, prop);
    }
    else if(propHeader.isScalar())
    {
        Alembic::Abc::IScalarProperty prop(iParent, propHeader.getName());
        if(!prop.isConstant())
            return;
        addScalarProperty<PODTYPE>(data, prop);
    }
}

void BaseAlembicObject::fillPropertyMap(const Alembic::Abc::ICompoundProperty& iParent, QVariantMap& variantMap)
{
    if(!iParent.valid())
        return;
    std::size_t numProps = iParent.getNumProperties();
    for (std::size_t i = 0; i < numProps; ++i)
    {
        const Alembic::Abc::PropertyHeader & propHeader = iParent.getPropertyHeader(i);
        Alembic::AbcCoreAbstract::DataType dtype = propHeader.getDataType();

        switch(dtype.getPod())
        {
        case Alembic::Abc::kBooleanPOD:
            addProperty<bool>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kUint8POD:
            addProperty<quint8>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kUint16POD:
            addProperty<quint16>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kUint32POD:
            addProperty<quint32>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kUint64POD:
            addProperty<quint64>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kInt8POD:
            addProperty<qint8>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kInt16POD:
            addProperty<qint16>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kInt32POD:
            addProperty<qint32>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kInt64POD:
            addProperty<qint64>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kFloat16POD:
            addProperty<float>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kFloat32POD:
            addProperty<float>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kFloat64POD:
            addProperty<double>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kStringPOD:
            addProperty<std::string>(variantMap, iParent, propHeader); break;
        case Alembic::Abc::kUnknownPOD:
        default:
            break;
        }
    }
}

}
