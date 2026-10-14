#pragma once

#include <Core/BoundingBox.hpp>

#include <QMatrix4x4>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector3D>
#include <memory>
#include <unordered_map>
#include <vector>
#include <qqml.h>
#include <QFutureWatcher>

#include <aliceVision/types.hpp>
#include <aliceVision/geometry/Pose3.hpp>
#include <aliceVision/camera/IntrinsicBase.hpp>

class CameraInfo;

/** @brief Base depth of the camera frustum shape, in world units (at camera scale 1). */
const float cameraDepth = 0.02f;
/** @brief Base radius of the panoramic camera sphere shape, in world units (at camera scale 1). */
const float sphereRadius = 0.05f;

/** @brief Reconstructed landmark. */
struct SfmDataPointInstance
{
    QVector3D position; /**< Position, in viewer world coordinates. */
    QVector3D color;    /**< RGB color, in [0, 1]. */
};

/** @brief Reconstructed camera of a view. */
struct SfmDataCameraInstance
{
    aliceVision::IndexT viewId = 0;      /**< View identifier. */
    aliceVision::IndexT frameId = 0;     /**< Frame identifier. */
    aliceVision::IndexT resectionId = 0; /**< Resection step at which the camera was localized. */
    QString path = "";                   /**< Image path. */

    aliceVision::geometry::Pose3 camera_T_world;                     /**< World-to-camera pose, in AliceVision convention. */
    std::shared_ptr<aliceVision::camera::IntrinsicBase> intrinsics; /**< Camera intrinsics. */
};

/**
 * @brief Convert an AliceVision pose to a view matrix in viewer (OpenGL) convention.
 *
 * The Y and Z axes are flipped on both sides of the pose to go from the computer-vision
 * convention (Y down, Z forward) to the OpenGL one (Y up, Z backward).
 *
 * @param[in] pose World-to-camera pose.
 * @return The corresponding view matrix.
 */
QMatrix4x4 toQMatrix4x4(const aliceVision::geometry::Pose3& pose);

/** @brief Output of the background SfM data loading task. */
struct SfmDataContent
{
    std::vector<SfmDataPointInstance> points;                                       /**< Landmarks. */
    std::vector<SfmDataCameraInstance> cameras;                                     /**< Reconstructed cameras, in file order. */
    std::unordered_map<aliceVision::IndexT, SfmDataCameraInstance> cameraPerViewId; /**< Reconstructed cameras, by view id. */

    quint32 maxResectionId = 0; /**< Largest resection id among the cameras. */
    QString errorString;        /**< Error message when loading failed. */
    bool valid = false;         /**< True when loading succeeded. */

    BoundingBox pointCloudBoundingBox;   /**< Bounds of the landmark positions; empty when there are none. */
    BoundingBox cameraCenterBoundingBox; /**< Bounds of the camera centers; empty when there are none. */
};

/**
 * @brief AliceVision SfM data file loaded asynchronously, shareable between layers.
 *
 * Exposes the landmarks and reconstructed cameras, converted to the viewer convention, and
 * QML helpers to query a camera by view id.
 */
class SfmDataObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Path or file:// URL of the SfM data file to load. */
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    /** @brief True while a file is being loaded. */
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    /** @brief Error message of the last failed load, empty on success. */
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    /** @brief True when SfM data is loaded. */
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)
    /** @brief Number of landmarks. */
    Q_PROPERTY(int pointCount READ pointCount NOTIFY pointCountChanged)
    /** @brief Number of reconstructed cameras. */
    Q_PROPERTY(int cameraCount READ cameraCount NOTIFY cameraCountChanged)
    Q_PROPERTY(quint32 maxResectionId READ maxResectionId NOTIFY maxResectionIdChanged)
    /** @brief Cameras with a larger resection id are hidden by SfmDataLayer. */
    Q_PROPERTY(quint32 limitResectionId READ limitResectionId WRITE setLimitResectionId NOTIFY limitResectionIdChanged)
    /** @brief Landmark bounds as [min, max] QVector3D values, or an empty list. */
    Q_PROPERTY(QVariantList pointCloudBoundingBox READ pointCloudBoundingBox NOTIFY pointCloudBoundingBoxChanged)
    /** @brief Camera center bounds as [min, max] QVector3D values, or an empty list. */
    Q_PROPERTY(QVariantList cameraCenterBoundingBox READ cameraCenterBoundingBox NOTIFY cameraCenterBoundingBoxChanged)

  public:
    /**
     * @brief Build an empty object.
     * @param[in] parent Optional Qt parent object.
     */
    explicit SfmDataObject(QObject* parent = nullptr);
    /** @brief Wait for any pending load, then destroy the object. */
    ~SfmDataObject() override;

    /** @brief Get the SfM data path. */
    QString source() const
    {
        return _source;
    }
    /**
     * @brief Start loading a new SfM data file in the background.
     * @param[in] path Path or file:// URL of the file.
     */
    void setSource(const QString& path);

    /** @brief Returns true while a file is being loaded. */
    bool loading() const
    {
        return _loading;
    }
    /** @brief Get the error message of the last failed load. */
    QString errorString() const
    {
        return _errorString;
    }
    /** @brief Returns true when SfM data is loaded. */
    bool valid() const
    {
        return _content && _content->valid;
    }
    /** @brief Returns the number of landmarks. */
    int pointCount() const
    {
        return _content ? static_cast<int>(_content->points.size()) : 0;
    }
    /** @brief Returns the number of reconstructed cameras. */
    int cameraCount() const
    {
        return _content ? static_cast<int>(_content->cameraPerViewId.size()) : 0;
    }
    quint32 maxResectionId() const
    {
        return _content ? _content->maxResectionId : 0;
    }
    /** @brief Get the resection id above which cameras are hidden. */
    quint32 limitResectionId() const
    {
        return _limitResectionId;
    }
    /**
     * @brief Set the resection id above which cameras are hidden.
     * @param[in] value Resection id limit.
     */
    void setLimitResectionId(quint32 value);

    /** @brief Returns the landmark bounds as [min, max], or an empty list. */
    QVariantList pointCloudBoundingBox() const
    {
        QVariantList box;
        if (!_content || !_content->pointCloudBoundingBox.isValid())
        {
            return box;
        }
        box << QVariant::fromValue(_content->pointCloudBoundingBox.min());
        box << QVariant::fromValue(_content->pointCloudBoundingBox.max());
        return box;
    }

    /** @brief Returns the camera center bounds as [min, max], or an empty list. */
    QVariantList cameraCenterBoundingBox() const
    {
        QVariantList box;
        if (!_content || !_content->cameraCenterBoundingBox.isValid())
        {
            return box;
        }
        box << QVariant::fromValue(_content->cameraCenterBoundingBox.min());
        box << QVariant::fromValue(_content->cameraCenterBoundingBox.max());
        return box;
    }

    /** @brief C++ API for layers — call only after dataReady() has been emitted. */
    const SfmDataContent& content() const
    {
        return *_content;
    }

    /** @brief Whether a camera transform is available for the given view id. */
    Q_INVOKABLE bool hasCameraTransform(quint32 viewId) const;
    /**
     * @brief Get the view matrix of a camera, in viewer convention.
     * @param[in] viewId View identifier.
     * @return The view matrix, or identity if the view has no camera.
     */
    Q_INVOKABLE QMatrix4x4 getCameraTransform(quint32 viewId) const;
    /**
     * @brief Get the center of a camera, in viewer world coordinates.
     * @param[in] viewId View identifier.
     * @return The camera center, or the origin if the view has no camera.
     */
    Q_INVOKABLE QVector3D getCameraCenter(quint32 viewId) const;
    /**
     * @brief Get the image path of a view.
     * @param[in] viewId View identifier.
     * @return The image path, or an empty string if the view has no camera.
     */
    Q_INVOKABLE QString getImagePath(quint32 viewId) const;
    /**
     * @brief Build an AVCameraInfo matching the intrinsics of a view.
     * @param[in] viewId View identifier.
     * @return A new camera owned by the JavaScript engine, or nullptr if the view has no camera or its intrinsics are not pinhole.
     */
    Q_INVOKABLE CameraInfo* getCameraInfo(quint32 viewId) const;

  signals:
    /** @brief Emitted when the source path changes. */
    void sourceChanged();
    /** @brief Emitted when loading starts or ends. */
    void loadingChanged();
    /** @brief Emitted after each load completes. */
    void errorStringChanged();
    /** @brief Emitted when the validity changes. */
    void validChanged();
    /** @brief Emitted when the landmark count changes. */
    void pointCountChanged();
    /** @brief Emitted when the camera count changes. */
    void cameraCountChanged();
    void maxResectionIdChanged();
    /** @brief Emitted when the resection id limit changes. */
    void limitResectionIdChanged();
    /** @brief Emitted when the landmark bounds change. */
    void pointCloudBoundingBoxChanged();
    /** @brief Emitted when the camera center bounds change. */
    void cameraCenterBoundingBoxChanged();
    /** @brief Emitted when SfM data was loaded successfully. */
    void dataReady();

  private slots:
    /** @brief Collect the background load result and notify. */
    void onLoadFinished();

  private:
    /** @brief SfM data path. */
    QString _source;
    /** @brief Loading flag. */
    bool _loading = false;
    /** @brief Error message of the last failed load. */
    QString _errorString;
    /** @brief Resection id above which cameras are hidden. */
    quint32 _limitResectionId = 0;
    /** @brief Last loaded content. */
    std::unique_ptr<SfmDataContent> _content = std::make_unique<SfmDataContent>();
    /** @brief Watcher of the background load task. */
    QFutureWatcher<std::unique_ptr<SfmDataContent>> _watcher;
};
