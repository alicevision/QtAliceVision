#include "SfmDataEntity.hpp"
#include "IOThread.hpp"

#include "CameraLocatorEntity.hpp"
#include "PointCloudEntity.hpp"

#include <aliceVision/geometry/Pose3.hpp>

#include <Qt3DRender/QEffect>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QShaderProgram>
#include <Qt3DRender/QObjectPicker>
#include <Qt3DRender/QPickEvent>
#include <Qt3DRender/QDebugOverlay>
#include <Qt3DExtras/QPerVertexColorMaterial>
#include <Qt3DRender/QGraphicsApiFilter>
#include <QFile>

namespace sfmdataentity {

SfmDataEntity::SfmDataEntity(Qt3DCore::QNode* parent)
  : Qt3DCore::QEntity(parent),
    _pointSizeParameter(new Qt3DRender::QParameter),
    _ioThread(new IOThread())
{
    connect(_ioThread.get(), &IOThread::finished, this, &SfmDataEntity::onIOThreadFinished);
    createMaterials();
}

void SfmDataEntity::setSource(const QUrl& value)
{
    if (_source == value)
        return;
    _source = value;
    loadSfmData();
    Q_EMIT sourceChanged();
}

void SfmDataEntity::setPointSize(const float& value)
{
    if (_pointSize == value)
    {
        return;
    }

    _pointSize = value;
    _pointSizeParameter->setValue(value);
    _cloudMaterial->setEnabled(_pointSize > 0.0f);

    Q_EMIT pointSizeChanged();
}

void SfmDataEntity::setLocatorScale(const float& value)
{
    if (_locatorScale == value)
    {
        return;
    }

    _locatorScale = value;
    scaleLocators();

    Q_EMIT locatorScaleChanged();
}

void SfmDataEntity::scaleLocators() const
{
    
    for (auto* entity : _cameras)
    {
        for (auto* transform : entity->findChildren<Qt3DCore::QTransform*>(QString(), Qt::FindDirectChildrenOnly))
        {
            if (entity->viewId() == _selectedViewId)
            {
                entity->updateColors(0.f, 0.f, 1.f);
                transform->setScale(_locatorScale * 1.5f);
            }
            else
            {
                entity->updateColors(1.f, 1.f, 1.f);
                transform->setScale(_locatorScale);
            }
        }
    }
}

void SfmDataEntity::setSelectedViewId(const aliceVision::IndexT& viewId)
{
    if (_selectedViewId == viewId)
    {
        return;
    }

    bool previousReset = _selectedViewId == 0 ? true : false;
    bool newUpdated = false;
    for (auto* entity : _cameras)
    {
        if (entity->viewId() == _selectedViewId)  // Previously selected camera: the scale must be reset
        {
            entity->updateColors(1.f, 1.f, 1.f);
            for (auto* transform : entity->findChildren<Qt3DCore::QTransform*>(QString(), Qt::FindDirectChildrenOnly))
            {
                transform->setScale(_locatorScale);
            }
            previousReset = true;
        }
        else if (entity->viewId() == viewId)  // Newly selected camera: the scale must be enlarged
        {
            entity->updateColors(0.f, 0.f, 1.f);
            for (auto* transform : entity->findChildren<Qt3DCore::QTransform*>(QString(), Qt::FindDirectChildrenOnly))
            {
                transform->setScale(_locatorScale * 1.5f);
            }
            newUpdated = true;
        }

        if (previousReset && newUpdated)
        {
            break;
        }
    }
    _selectedViewId = viewId;

    Q_EMIT selectedViewIdChanged();
}

void SfmDataEntity::setResectionId(const aliceVision::IndexT& value)
{
    if (_resectionId == value)
    {
        return;
    }

    _resectionId = value;
    for (auto* entity : _cameras)
    {
        if (entity->resectionId() > _resectionId && _displayResections)
        {
            entity->setEnabled(false);
        }
        else
        {
            entity->setEnabled(true);
        }
    }

    Q_EMIT resectionIdChanged();
}

void SfmDataEntity::setDisplayResections(const bool value)
{
    if (_displayResections == value)
    {
        return;
    }

    _displayResections = value;
    // Re-enable all cameras both when the display of the resections is enabled and when it is disabled
    for (auto* entity : _cameras)
    {
        entity->setEnabled(true);
    }
}

void SfmDataEntity::createMaterials()
{
    using namespace Qt3DRender;
    using namespace Qt3DExtras;

    _cloudMaterial = new QMaterial(this);
    _cameraMaterial = new QPerVertexColorMaterial(this);

    // Configure cloud material
    auto effect = new QEffect();
    auto technique = new QTechnique();
    auto renderPass = new QRenderPass();
    auto shaderProgram = new QShaderProgram();

    technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::RHI);
    technique->graphicsApiFilter()->setMajorVersion(1);
    technique->graphicsApiFilter()->setMinorVersion(0);
    technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    shaderProgram->setShaderCode(QShaderProgram::Vertex, R"(#version 450
    layout(location = 0) in vec3 vertexPosition;
    layout(location = 1) in vec3 vertexColor;
    layout(location = 0) out vec3 color;
    layout(std140, binding = 0) uniform qt3d_render_view_uniforms {
        mat4 viewMatrix;
        mat4 projectionMatrix;
        mat4 uncorrectedProjectionMatrix;
        mat4 clipCorrectionMatrix;
        mat4 viewProjectionMatrix;
        mat4 inverseViewMatrix;
        mat4 inverseProjectionMatrix;
        mat4 inverseViewProjectionMatrix;
        mat4 viewportMatrix;
        mat4 inverseViewportMatrix;
        vec4 textureTransformMatrix;
        vec3 eyePosition;
        float aspectRatio;
        float gamma;
        float exposure;
        float time;
    };
    layout(std140, binding = 1) uniform qt3d_command_uniforms {
        mat4 modelMatrix;
        mat4 inverseModelMatrix;
        mat4 modelViewMatrix;
        mat3 modelNormalMatrix;
        mat4 inverseModelViewMatrix;
        mat4 mvp;
        mat4 inverseModelViewProjectionMatrix;
    };
    layout(std140, binding = 2) uniform custom_ubo {
        float pointSize;
    };

    void main()
    {
        color = vertexColor;
        gl_Position = mvp * vec4(vertexPosition, 1.0);
        gl_PointSize = max(viewportMatrix[1][1] * projectionMatrix[1][1] * pointSize / gl_Position.w, 1.0);
    }
    )");

    shaderProgram->setShaderCode(QShaderProgram::Fragment, R"(#version 450
    layout(location = 0) out vec4 fragColor;
    layout(location = 0) in vec3 color;
    void main()
    {
        fragColor = vec4(color, 1.0);
    }
    )");

    // Add a pointSize uniform
    _pointSizeParameter->setName("pointSize");
    _pointSizeParameter->setValue(_pointSize);
    _cloudMaterial->addParameter(_pointSizeParameter);

    // Build the material
    renderPass->setShaderProgram(shaderProgram);
    technique->addRenderPass(renderPass);
    effect->addTechnique(technique);
    _cloudMaterial->setEffect(effect);
}

void SfmDataEntity::clear()
{
    // Clear entity (remove direct children & all components)
    auto entities = findChildren<QEntity*>(QString(), Qt::FindDirectChildrenOnly);
    for (auto entity : entities)
    {
        entity->setParent(static_cast<QNode*>(nullptr));
        entity->deleteLater();
    }
    for (auto& component : components())
    {
        removeComponent(component);
    }

    _cameras.clear();
    _pointClouds.clear();
}

// Private
void SfmDataEntity::loadSfmData()
{
    clear();

    if (_source.isEmpty())
    {
        setStatus(SfmDataEntity::None);
        return;
    }

    setStatus(SfmDataEntity::Loading);

    _ioThread->read(_source);
}

void SfmDataEntity::onIOThreadFinished()
{
    const auto& sfmData = _ioThread->getSfmData();
    if (sfmData == aliceVision::sfmData::SfMData())
    {
        qCritical() << "[QmlSfmData] The SfMData has not been correctly initialized, the file may not be valid.";
        setStatus(SfmDataEntity::Error);
        return;
    }
    else if (sfmData.getLandmarks().empty() && sfmData.getPoses().empty())
    {
        qCritical() << "[QmlSfmData] The SfMData has been initialized but does not contain any 3D information.";
        setStatus(SfmDataEntity::Error);
        return;
    }
    else
    {
        Qt3DCore::QEntity* root = new Qt3DCore::QEntity(this);

        {
            PointCloudEntity* entity = new PointCloudEntity(root);
            entity->setData(sfmData.getLandmarks());
            entity->addComponent(_cloudMaterial);
        }

        for (const auto& pv : sfmData.getViews())
        {
            if (!sfmData.isPoseAndIntrinsicDefined(pv.second.get()))
            {
                continue;
            }

            aliceVision::IndexT intrinsicId = pv.second->getIntrinsicId();
            auto intrinsic = sfmData.getIntrinsicSharedPtr(intrinsicId);
            float hfov = static_cast<float>(intrinsic->getHorizontalFov());
            float vfov = static_cast<float>(intrinsic->getVerticalFov());

            CameraLocatorEntity* entity = new CameraLocatorEntity(pv.first, pv.second->getResectionId(), hfov, vfov, root);
            entity->addComponent(_cameraMaterial);
            entity->setTransform(sfmData.getPose(*pv.second).getTransform().getHomogeneous());
            entity->setObjectName(std::to_string(pv.first).c_str());
            if (entity->resectionId() > _resectionId && _displayResections)
            {
                entity->setEnabled(false);
            }
        }

        _cameras = findChildren<CameraLocatorEntity*>();
        _pointClouds = findChildren<PointCloudEntity*>();

        scaleLocators();

        setStatus(SfmDataEntity::Ready);
    }


    _ioThread->clear();

    Q_EMIT pointCloudsChanged();
    Q_EMIT camerasChanged();
}

}  // namespace sfmdataentity
