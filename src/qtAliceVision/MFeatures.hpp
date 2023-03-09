#pragma once

#include <aliceVision/types.hpp>
#include <aliceVision/feature/feature.hpp>

#include <QObject>
#include <QRunnable>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include <map>
#include <vector>
#include <string>

namespace qtAliceVision
{

using FeaturesPerViewPerDesc =
    std::map<std::string, std::map<aliceVision::IndexT, std::vector<aliceVision::feature::PointFeature>>>;

/**
 * @brief QObject wrapper around extracted features.
 * 
 * Given a folder containing extracted features,
 * the role of an MFeatures instance is to load the features from disk.
 * Describer types and view IDs to load must also be specified.
 * This task is done asynchronously to avoid freezing the UI.
 * 
 * MFeatures objects are accessible from QML
 * and can be manipulated through their properties.
 * 
 * Note:
 * for a given describer type and view ID,
 * features are stored in an array-like structure
 * and the ID a a feature corresponds to its index in this array.
 */
class MFeatures : public QObject
{
    Q_OBJECT

    /// Data properties

    // Path to folder containing the features
    Q_PROPERTY(QUrl featureFolder MEMBER _featureFolder NOTIFY featureFolderChanged)
    // View IDs to load
    Q_PROPERTY(QVariantList viewIds MEMBER _viewIds NOTIFY viewIdsChanged)
    // Describer types to load
    Q_PROPERTY(QVariantList describerTypes MEMBER _describerTypes NOTIFY describerTypesChanged)
    /// The list of features information (per view, per describer) for UI
    Q_PROPERTY(QVariantMap featuresInfo READ featuresInfo NOTIFY featuresInfoChanged)

    /// Status

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    /// Status Enum

    enum Status
    {
        None = 0,
        Loading,
        Ready,
        Error
    };
    Q_ENUM(Status)

    /// Slots

    Q_SLOT void load();
    Q_SLOT void onFeaturesReady(FeaturesPerViewPerDesc* featuresPerViewPerDesc);

    /// Signals

    Q_SIGNAL void featureFolderChanged();
    Q_SIGNAL void describerTypesChanged();
    Q_SIGNAL void viewIdsChanged();
    Q_SIGNAL void featuresInfoChanged();
    Q_SIGNAL void featuresChanged();
    Q_SIGNAL void statusChanged(Status status);

    /// Public methods

    MFeatures();
    MFeatures(const MFeatures& other) = default;
    ~MFeatures() override;

    FeaturesPerViewPerDesc& rawData() { return *_featuresPerViewPerDesc; }
    const FeaturesPerViewPerDesc& rawData() const { return *_featuresPerViewPerDesc; }
    const FeaturesPerViewPerDesc* rawDataPtr() const { return _featuresPerViewPerDesc; }

    const std::vector<aliceVision::feature::PointFeature>& getFeatures(
        const std::string& describerType, const aliceVision::IndexT& viewId) const;
    
    float getMinFeatureScale(const std::string& describerType) const;
    float getMaxFeatureScale(const std::string& describerType) const;

    /**
     * @brief Get MFeatures status
     * @see MFeatures status enum
     * @return MFeatures status enum
     */
    Status status() const { return _status; }

    void setStatus(Status status);

    const QVariantMap& featuresInfo() const
    {
        return _featuresInfo;
    }

private:
    /// Private methods

    void updateMinMaxFeatureScale();

    void updateFeaturesInfo();

    /// Private members

    // inputs
    QUrl _featureFolder;
    QVariantList _viewIds;
    QVariantList _describerTypes;

    bool _needReload = false;

    // internal data
    std::map<std::string, float> _minFeatureScalePerDesc;
    std::map<std::string, float> _maxFeatureScalePerDesc;
    QVariantMap _featuresInfo;
    FeaturesPerViewPerDesc* _featuresPerViewPerDesc = nullptr;

    /// status
    Status _status = MFeatures::None;
};

/**
 * @brief QRunnable object dedicated to loading features using AliceVision.
 */
class FeaturesIORunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:
    FeaturesIORunnable(const std::string& folder,
                       const std::vector<aliceVision::IndexT>& viewIds,
                       const std::vector<std::string>& describerTypes)
        : _folder(folder), _viewIds(viewIds), _describerTypes(describerTypes)
    {
    }

    /// Load features based on input parameters
    Q_SLOT void run() override;

    /**
     * @brief  Emitted when features have been loaded and Features objects created.
     * @warning Features objects are not parented - their deletion must be handled manually.
     *
     * @param features the loaded Features list
     */
    Q_SIGNAL void resultReady(FeaturesPerViewPerDesc* featuresPerViewPerDesc);

private:
    std::string _folder;
    std::vector<aliceVision::IndexT> _viewIds;
    std::vector<std::string> _describerTypes;
};

} // namespace qtAliceVision
