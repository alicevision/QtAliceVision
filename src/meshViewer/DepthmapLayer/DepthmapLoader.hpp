#pragma once

#include <DepthmapLayer/DepthmapData.hpp>
#include <QString>
#include <memory>

/** @brief Loads a depth map (+ optional similarity map) EXR pair into an indexed point/triangle mesh. */
class DepthmapLoader
{
  public:
    /**
     * @brief Load a depth map from @p path.
     * @param path Path to either the "depthMap" or "simMap" file of the pair; the companion
     *             file is resolved automatically from the filename, as in DepthMapEntity.
     */
    static std::unique_ptr<DepthmapData> load(const QString& path);

  private:
    static void computeBounds(DepthmapData& data);
};
