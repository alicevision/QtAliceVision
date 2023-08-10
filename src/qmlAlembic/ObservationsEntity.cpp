#include "ObservationsEntity.hpp"
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DExtras/QPhongAlphaMaterial>

#include <aliceVision/sfmDataIO/AlembicImporter.cpp>

using namespace Qt3DRender;
using namespace aliceVision;

namespace abcentity
{

static const auto& TC_INDEX_ATTRIBUTE_NAME = "Tc-observations indices";
static const auto& RC_INDEX_ATTRIBUTE_NAME = "Rc-observations indices";

/**
 * @brief encapsulates sufficient and fast retrievable information on a landmark observed by a view
 */
struct LandmarkPerView
{
    LandmarkPerView() = default;

    LandmarkPerView(const IndexT& landmarkId, const sfmData::Landmark& landmark,
                    const sfmData::Observation& observation)
        : landmarkId(landmarkId)
        , landmark(landmark)
        , observation(observation)
    {
    }

    const uint& landmarkId;
    const sfmData::Landmark& landmark;
    const sfmData::Observation& observation;
};

// helper functions

QGeometryRenderer* createGeometryRenderer(const QByteArray& positionData, const std::string& indexAttributeName);
QMaterial* createMaterial(const float& r, const float& g, const float& b, const float& a);

ObservationsEntity::ObservationsEntity(std::string source, Qt3DCore::QNode* parent)
    : BaseAlembicObject(parent)
    , _source(source)
{
    using sfmDataIO::ESfMData;

    // read relevant SfM data
    sfmDataIO::Load(_sfmData, _source, ESfMData(
        ESfMData::VIEWS | ESfMData::EXTRINSICS | ESfMData::STRUCTURE | 
        ESfMData::OBSERVATIONS | ESfMData::OBSERVATIONS_WITH_FEATURES));

    // populate _landmarksPerView from the read SfM data
    fillLandmarksPerViews();
}

void ObservationsEntity::setData(const Alembic::Abc::IObject& iObj)
{
    // contains the 3D coordinates of vertices in space in the following order:
    // - the landmarks
    // - the view cameras
    QByteArray positionData;

    fillBytesData(iObj, positionData);

    // contains the connections between the selected view and its corresponding
    // observed landmarks
    auto* rcGeometry = createGeometryRenderer(positionData, RC_INDEX_ATTRIBUTE_NAME);
    // contains the connections between the observed landmarks of the current view
    // and their corresponding observing view cameras
    auto* tcGeometry = createGeometryRenderer(positionData, TC_INDEX_ATTRIBUTE_NAME);

    // create separate QEntity objects to have different material colors

    auto* tcEntity = new QEntity(this);
    tcEntity->addComponent(tcGeometry);
    tcEntity->addComponent(createMaterial(0, 1, 0, .7f));

    auto* rcEntity = new QEntity(this);
    rcEntity->addComponent(rcGeometry);
    rcEntity->addComponent(createMaterial(0, 0, 1, .7f));
}

void ObservationsEntity::update(const IndexT& viewId, const QVariantMap& viewer2DInfo)
{
    QByteArray tcIndexData;
    QByteArray rcIndexData;
    size_t tcIndexSize = 0;
    size_t rcIndexCount = 0;

    const auto& it = _landmarksPerView.find(viewId);
    if (it != _landmarksPerView.end())
    {
        // 2D view bounds
        const float& scale = viewer2DInfo["scale"].toFloat();
        const float& x1 = -viewer2DInfo["x"].toFloat() / scale;
        const float& y1 = -viewer2DInfo["y"].toFloat() / scale;
        const float& x2 = viewer2DInfo["width"].toFloat() / scale + x1;
        const float& y2 = viewer2DInfo["height"].toFloat() / scale + y1;

        const auto& totalNbObservations = it->second.first;
        const auto& totalNbLandmarks = it->second.second.size();

        tcIndexData.resize(static_cast<int>(totalNbObservations * sizeof(uint) * 2));
        rcIndexData.resize(static_cast<int>(totalNbLandmarks * sizeof(uint) * 2));
        char* tcIndexIt = tcIndexData.data();
        uint* rcIndexIt = reinterpret_cast<uint*>(rcIndexData.data());

        const auto& rcVertexPos = _viewId2vertexPos.at(viewId);
        for (auto& landmarkPerView : it->second.second)
        {
            const auto& coords2D = landmarkPerView.observation.x;
            // if not observed in viewer 2D
            if (coords2D.x() < x1 || coords2D.x() > x2 || coords2D.y() < y1 || coords2D.y() > y2)
                continue;

            *rcIndexIt++ = rcVertexPos;
            *rcIndexIt++ = landmarkPerView.landmarkId;
            rcIndexCount += 2;

            auto itt = _indexBytesByLandmark.data();
            std::advance(itt, _landmarkId2IndexRange[landmarkPerView.landmarkId].first * sizeof(uint));
            const auto& d = _landmarkId2IndexRange[landmarkPerView.landmarkId].second * sizeof(uint);
            memcpy(tcIndexIt, itt, d);
            std::advance(tcIndexIt, d);
            tcIndexSize += d;
        }
        tcIndexData.resize(static_cast<int>(tcIndexSize));
        rcIndexData.resize(static_cast<int>(rcIndexCount * sizeof(uint)));
    }

    this->findChild<Qt3DRender::QAttribute*>(TC_INDEX_ATTRIBUTE_NAME)->buffer()->setData(tcIndexData);
    this->findChild<Qt3DRender::QAttribute*>(TC_INDEX_ATTRIBUTE_NAME)
        ->setCount(static_cast<uint>(tcIndexSize / sizeof(uint)));

    this->findChild<Qt3DRender::QAttribute*>(RC_INDEX_ATTRIBUTE_NAME)->buffer()->setData(rcIndexData);
    this->findChild<Qt3DRender::QAttribute*>(RC_INDEX_ATTRIBUTE_NAME)->setCount(static_cast<uint>(rcIndexCount));
}

void ObservationsEntity::fillLandmarksPerViews()
{
    for (const auto& landIt : _sfmData.getLandmarks())
    {
        for (const auto& obsIt : landIt.second.observations)
        {
            IndexT viewId = obsIt.first;
            auto& [totalNbObservations, landmarksSet] = _landmarksPerView[viewId];
            totalNbObservations += landIt.second.observations.size();
            landmarksSet.push_back(LandmarkPerView(landIt.first, landIt.second, obsIt.second));
        }
    }
}

void ObservationsEntity::fillBytesData(const Alembic::Abc::IObject& iObj, QByteArray& positionData)
{
     using namespace Alembic::Abc;
     using namespace Alembic::AbcGeom;
     using sfmDataIO::AV_UInt32ArraySamplePtr;

     IPoints points(iObj, kWrapExisting);
     IPointsSchema schema = points.getSchema();

     // -------------- read position data ----------------------

     P3fArraySamplePtr positionsLandmarks = schema.getValue().getPositions();
     const auto& nLandmarks = positionsLandmarks->size();
     const auto& nViews = _sfmData.getViews().size();
     positionData.resize(static_cast<int>((nLandmarks + nViews) * 3 * sizeof(float)));
     // copy positions of landmarks
     memcpy(positionData.data(), (const char*)positionsLandmarks->get(), nLandmarks * 3 * sizeof(float));
     // copy positions of view cameras and construct _viewId2vertexPos
     {
          auto* positionsIt = positionData.data();
          uint viewPosIdx = static_cast<uint>(nLandmarks);
          // view camera positions are added after landmarks'
          positionsIt += 3 * sizeof(float) * nLandmarks;
          for (const auto& viewIt : _sfmData.getViews())
          {
             _viewId2vertexPos[viewIt.first] = viewPosIdx++;
             aliceVision::Vec3f center = _sfmData.getPose(*viewIt.second).getTransform().center().cast<float>();
             // graphics to open-gl coordinates system
             center.z() *= -1;
             center.y() *= -1;
             memcpy(positionsIt, reinterpret_cast<char*>(center.data()), 3 * sizeof(float));
             positionsIt += 3 * sizeof(float);
          }
     }

     // -------------- read Tc index data ----------------------

     {
          ICompoundProperty userProps = aliceVision::sfmDataIO::getAbcUserProperties(schema);
          AV_UInt32ArraySamplePtr sampleVisibilitySize(userProps, "mvg_visibilitySize");
          AV_UInt32ArraySamplePtr sampleVisibilityViewId(userProps, "mvg_visibilityViewId");

         _indexBytesByLandmark.resize(static_cast<int>(2 * sizeof(uint) * sampleVisibilityViewId.size()));
         _landmarkId2IndexRange.resize(sampleVisibilityViewId.size());
         uint* indices = reinterpret_cast<uint*>(_indexBytesByLandmark.data());
         uint offset = 0;
         size_t obsGlobalIndex = 0;
         for (uint point3d_i = 0; point3d_i < sampleVisibilitySize.size(); ++point3d_i)
         {
             // Number of observation for this 3d point
             const size_t visibilitySize = sampleVisibilitySize[point3d_i];
             const auto& delta = static_cast<uint>(visibilitySize * 2);
             _landmarkId2IndexRange[point3d_i] = std::pair<uint, uint>(offset, delta);
             offset += delta;
             for (size_t obs_i = 0; obs_i < visibilitySize; ++obs_i, ++obsGlobalIndex)
             {
                 *indices++ = point3d_i;
                 *indices++ = _viewId2vertexPos.at(static_cast<IndexT>(sampleVisibilityViewId[obsGlobalIndex]));
             }
         }
     }
}

QGeometryRenderer* createGeometryRenderer(const QByteArray& positionData, const std::string& indexAttributeName)
{
     // create a new geometry renderer
     auto customMeshRenderer = new QGeometryRenderer;
     // the corresponding geometry consisting of:
     // - a position attribute defining the 3D points
     // - an index attribute defining the connectivity between points
     auto customGeometry = new QGeometry;

     // mesh
     customMeshRenderer->setGeometry(customGeometry);
     customMeshRenderer->setPrimitiveType(QGeometryRenderer::Lines);

     // ------------- 3D points -------------------

     // the vertices buffer
     auto* vertexDataBuffer = new QBuffer;
     vertexDataBuffer->setData(positionData);
     // the position attribute
     auto* positionAttribute = new QAttribute;
     positionAttribute->setBuffer(vertexDataBuffer);
     positionAttribute->setAttributeType(QAttribute::VertexAttribute);
     positionAttribute->setVertexBaseType(QAttribute::Float);
     positionAttribute->setVertexSize(3); // space dimension
     positionAttribute->setByteOffset(0);
     const auto& nbBytesPerVertex = 3 * sizeof(float);
     positionAttribute->setByteStride(static_cast<uint>(nbBytesPerVertex));
     positionAttribute->setCount(static_cast<uint>(positionData.size() / nbBytesPerVertex));
     positionAttribute->setName(QAttribute::defaultPositionAttributeName());
     customGeometry->addAttribute(positionAttribute); // add to the geometry

     // ------------- connectivity between 3D points -------------------

     // byte array containing pairs of indices to connected 3D points in poisition attribute
     QByteArray indexBytes;
     // the indices buffer
     auto* indexBuffer = new QBuffer;
     indexBuffer->setData(indexBytes);
     // the index attribute
     auto* indexAttribute = new QAttribute;
     indexAttribute->setBuffer(indexBuffer);
     indexAttribute->setAttributeType(QAttribute::IndexAttribute);
     indexAttribute->setVertexBaseType(QAttribute::UnsignedInt);
     indexAttribute->setCount(0); // index data not yet available
     // set name for accessing the attribute and filling data later
     indexAttribute->setObjectName(QString::fromStdString(indexAttributeName));
     customGeometry->addAttribute(indexAttribute); // add to the geometry

     return customMeshRenderer;
}

QMaterial* createMaterial(const float& r, const float& g, const float& b, const float& a)
{
     // material for the connections between 3D points
     auto* material = new Qt3DExtras::QPhongAlphaMaterial;
     material->setAmbient(QColor::fromRgbF(r, g, b));
     material->setDiffuse(QColor::fromRgbF(r, g, b));
     material->setSpecular(QColor::fromRgbF(r, g, b));
     material->setShininess(0.0f);
     material->setAlpha(a);

     return material;
}

} // namespace
