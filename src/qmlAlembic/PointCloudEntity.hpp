#pragma once

#include "BaseAlembicObject.hpp"


namespace abcentity
{

class PointCloudEntity : public BaseAlembicObject
{
    Q_OBJECT

public:
    explicit PointCloudEntity(Qt3DCore::QNode* = nullptr);
    ~PointCloudEntity() override = default;

public:
    void setData(const Alembic::Abc::IObject&);
};

} // namespace
