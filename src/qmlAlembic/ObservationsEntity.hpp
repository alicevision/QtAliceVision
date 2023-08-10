#pragma once

#include "BaseAlembicObject.hpp"
#include <aliceVision/sfmData/SfMData.hpp>


namespace abcentity
{

struct LandmarkPerView;

class ObservationsEntity : public BaseAlembicObject
{
    Q_OBJECT

public:
    explicit ObservationsEntity(std::string source, Qt3DCore::QNode* parent = nullptr);
    ~ObservationsEntity() override = default;

    void setData(const Alembic::Abc::IObject&);
    void update(const aliceVision::IndexT& viewId, const QVariantMap& viewer2DInfo);

private:

    void fillBytesData(const Alembic::Abc::IObject& iObj, QByteArray& positionData);
    void fillLandmarksPerViews();

    // file path to SfM data
    std::string _source;
    // contains the indices of vertices in position data used to draw lines
    // between landmarks and their corresponding obsevations (ordered by landmark)
    QByteArray _indexBytesByLandmark;
    // each pair represents the offset position and count of indices (not bytes)
    // in _indexBytesByLandmark for a landmark
    std::vector<std::pair<uint, uint>> _landmarkId2IndexRange;
    // maps view id to index of the corresponding position for a view camera in position data
    std::map<aliceVision::IndexT, uint> _viewId2vertexPos;
    aliceVision::sfmData::SfMData _sfmData;
    // maps view id to a pair containing:
    // - the total number of observations of all landmarks observed by this view
    // - a vector of LandmarkPerView structures corresponding to the landmarks observed by this view
    stl::flat_map<aliceVision::IndexT, std::pair<size_t, std::vector<LandmarkPerView>>> _landmarksPerView;
};

} // namespace
