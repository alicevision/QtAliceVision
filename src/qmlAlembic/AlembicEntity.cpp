#include "AlembicEntity.hpp"
#include "IOThread.hpp"
#include "CameraLocatorEntity.hpp"
#include "PointCloudEntity.hpp"
#include "ObservationsEntity.hpp"
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QShaderProgram>
#include <Qt3DRender/QObjectPicker>
#include <Qt3DRender/QPickEvent>
#include <Qt3DExtras/QPerVertexColorMaterial>
#include <QFile>

using namespace Alembic::Abc;
using namespace Alembic::AbcGeom;

namespace abcentity
{

AlembicEntity::AlembicEntity(Qt3DCore::QNode* parent)
    : Qt3DCore::QEntity(parent)
    , _pointSizeParameter(new Qt3DRender::QParameter)
    , _ioThread(new IOThread())
{
    connect(_ioThread.get(), &IOThread::finished, this, &AlembicEntity::onIOThreadFinished);
    createMaterials();

    // trigger display repaint events of observations
    connect(this, &AlembicEntity::viewIdChanged, this, &AlembicEntity::updateObservations);
    connect(this, &AlembicEntity::displayObservationsChanged, this, &AlembicEntity::updateObservations);
    connect(this, &AlembicEntity::viewer2DInfoChanged, this, &AlembicEntity::updateObservations);
}

void AlembicEntity::setSource(const QUrl& value)
{
    if(_source == value)
        return;
    _source = value;
    loadAbcArchive();
    Q_EMIT sourceChanged();
}

void AlembicEntity::setPointSize(const float& value)
{
    if(_pointSize == value)
        return;
    _pointSize = value;
    _pointSizeParameter->setValue(value);
    _cloudMaterial->setEnabled(_pointSize > 0.0f);
    Q_EMIT pointSizeChanged();
}

void AlembicEntity::setLocatorScale(const float& value)
{
    if(_locatorScale == value)
        return;
    _locatorScale = value;
    scaleLocators();
    Q_EMIT locatorScaleChanged();
}

void AlembicEntity::setViewId(const int& value)
{
    if (_viewId == value)
        return;
    _viewId = value;
    Q_EMIT viewIdChanged();
}

void AlembicEntity::setDisplayObservations(const bool& value)
{
    if (_displayObservations == value)
        return;
    _displayObservations = value;
    Q_EMIT displayObservationsChanged();
}

void AlembicEntity::setViewer2DInfo(const QVariantMap& value)
{
    if (_viewer2DInfo == value)
        return;
    _viewer2DInfo = value;
    Q_EMIT viewer2DInfoChanged();
}

void AlembicEntity::scaleLocators() const
{
    for(auto* entity : _cameras)
    {
        for(auto* transform : entity->findChildren<Qt3DCore::QTransform*>())
            transform->setScale(_locatorScale);
    }
}

void AlembicEntity::updateObservations() const
{
    for (auto* entity : _observations)
    {
        entity->update(_viewId, _viewer2DInfo);
        if (entity->isEnabled() != _displayObservations)
            entity->setEnabled(_displayObservations);
    }
}

// private
void AlembicEntity::createMaterials()
{
    using namespace Qt3DRender;
    using namespace Qt3DExtras;

    _cloudMaterial = new QMaterial(this);
    _cameraMaterial = new QPerVertexColorMaterial(this);

    // configure cloud material
    auto effect = new QEffect;
    auto technique = new QTechnique;
    auto renderPass = new QRenderPass;
    auto shaderProgram = new QShaderProgram;

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

void AlembicEntity::clear()
{
    // clear entity (remove direct children & all components)
    auto entities = findChildren<QEntity*>(QString(), Qt::FindDirectChildrenOnly);
    for(auto entity : entities)
    {
        entity->setParent((QNode*)nullptr);
        entity->deleteLater();
    }
    for(auto& component : components())
        removeComponent(component);
    _cameras.clear();
    _pointClouds.clear();
    _observations.clear();
}

// private
void AlembicEntity::loadAbcArchive()
{
    clear();
    if(_source.isEmpty())
    {
        setStatus(AlembicEntity::None);
        return;
    }
    setStatus(AlembicEntity::Loading);
    _ioThread->read(_source);
}

void AlembicEntity::onIOThreadFinished()
{
    const auto& archive = _ioThread->archive();
    if(!archive.valid())
    {
        setStatus(AlembicEntity::Error);
        return;
    }
    // visit the abc tree
    try
    {
        visitAbcObject(archive.getTop(), this);

        // store pointers to cameras and point clouds
        _cameras = findChildren<CameraLocatorEntity*>();
        _pointClouds = findChildren<PointCloudEntity*>();
        _observations = findChildren<ObservationsEntity*>();

        // perform initial locator scaling
        scaleLocators();

        updateObservations();

        setStatus(AlembicEntity::Ready);
    }
    catch(...)
    {
        clear();
        setStatus(AlembicEntity::Error);
    }
    _ioThread->clear();
    Q_EMIT camerasChanged();
    Q_EMIT pointCloudsChanged();
    Q_EMIT observationsChanged();
}

// private
void AlembicEntity::visitAbcObject(const Alembic::Abc::IObject& iObj, QEntity* parent)
{
    using namespace Alembic::Abc;
    using namespace Alembic::AbcGeom;

    const auto createEntities = [&](const IObject&) -> std::vector<BaseAlembicObject*> {
        const MetaData& md = iObj.getMetaData();

        if(IPoints::matches(md))
        {
            std::vector<BaseAlembicObject*> entities(2);
            IPoints points(iObj, Alembic::Abc::kWrapExisting);
            PointCloudEntity* entity0 = new PointCloudEntity(parent);
            entity0->setData(iObj);
            entity0->addComponent(_cloudMaterial);
            entity0->fillArbProperties(points.getSchema().getArbGeomParams());
            entity0->fillUserProperties(points.getSchema().getUserProperties());
            entities[0] = entity0;
            ObservationsEntity* entity1 = new ObservationsEntity(_source.toLocalFile().toStdString(), parent);
            entity1->setData(iObj);
            entity1->fillArbProperties(points.getSchema().getArbGeomParams());
            entity1->fillUserProperties(points.getSchema().getUserProperties());
            entities[1] = entity1;
            return entities;
        }
        else if(IXform::matches(md))
        {
            IXform xform(iObj, kWrapExisting);
            BaseAlembicObject* entity = new BaseAlembicObject(parent);
            XformSample xs;
            xform.getSchema().get(xs);
            entity->setTransform(xs.getMatrix());
            entity->fillArbProperties(xform.getSchema().getArbGeomParams());
            entity->fillUserProperties(xform.getSchema().getUserProperties());
            return {entity};
        }
        else if(ICamera::matches(md))
        {
            ICamera cam(iObj, Alembic::Abc::kWrapExisting);
            CameraLocatorEntity* entity = new CameraLocatorEntity(parent);
            entity->addComponent(_cameraMaterial);
            entity->fillArbProperties(cam.getSchema().getArbGeomParams());
            entity->fillUserProperties(cam.getSchema().getUserProperties());
            return {entity};
        }
        else
        {
            // fallback: create empty object to preserve hierarchy
            return {new BaseAlembicObject(parent)};
        }
    };

    if(_skipHidden)
    {
        // Skip objects with visibilityProperty explicitly set to hidden
        const auto& prop = iObj.getProperties();

        if(prop.getPropertyHeader(kVisibilityPropertyName))
        {
            IVisibilityProperty visibilityProperty(prop, kVisibilityPropertyName);
            if(ObjectVisibility(visibilityProperty.getValue()) == kVisibilityHidden)
                return;
        }
    }

    std::vector<BaseAlembicObject*> entities = createEntities(iObj);

    for (int j = 0; j < entities.size(); j++)
    {
        auto entity = entities[j];
        entity->setObjectName((iObj.getName() + std::to_string(j)).c_str());

        // visit children
        for (size_t i = 0; i < iObj.getNumChildren(); i++)
            visitAbcObject(iObj.getChild(i), entity);
    }
}

} // namespace
