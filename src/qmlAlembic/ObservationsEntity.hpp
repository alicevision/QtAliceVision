#pragma once

#include "BaseAlembicObject.hpp"
#include <aliceVision/sfmData/SfMData.hpp>

using namespace aliceVision;

namespace abcentity
{

/**
 * @brief encapsulates sufficient and fast retrievable information on a landmark observed by a view
 */
struct LandmarkPerView
{
    LandmarkPerView() = default;

    LandmarkPerView(const IndexT& landmarkId_, const sfmData::Landmark& landmark_,
                    const sfmData::Observation& observation_)
        : landmarkId(landmarkId_)
        , landmark(landmark_)
        , observation(observation_)
    {
    }

    const uint& landmarkId;
    const sfmData::Landmark& landmark;
    const sfmData::Observation& observation;
};

class ObservationsEntity : public BaseAlembicObject
{
    Q_OBJECT

public:
    explicit ObservationsEntity(std::string source, Qt3DCore::QNode* parent = nullptr);
    ~ObservationsEntity() override = default;

    void setData();
    void update(const IndexT& viewId, const QVariantMap& viewer2DInfo);

private:

    void fillBytesData(QByteArray& positionData);
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
    std::map<IndexT, uint> _viewId2vertexPos;
    sfmData::SfMData _sfmData;
    // maps view id to a pair containing:
    // - the total number of observations of all landmarks observed by this view
    // - a vector of LandmarkPerView structures corresponding to the landmarks observed by this view
    stl::flat_map<IndexT, std::pair<size_t, std::vector<LandmarkPerView>>> _landmarksPerView;
};

} // namespace
