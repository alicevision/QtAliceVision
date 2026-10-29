#pragma once

#include <Core/LayerItem.hpp>
#include <DepthmapLayer/DepthmapRenderable.hpp>
#include <DepthmapLayer/DepthmapData.hpp>
#include <QFutureWatcher>

/** @brief Layer that asynchronously loads an AliceVision depth map and draws it as a colored mesh. */
class DepthmapLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Path of the depthMap or simMap file to load. */
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    /** @brief True while a depth map is being loaded. */
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    /** @brief Error message of the last failed load, empty on success. */
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

  public:
    /**
     * @brief Build an empty layer.
     * @param[in] parent Optional Qt parent object.
     */
    explicit DepthmapLayer(QObject* parent = nullptr);
    /** @brief Wait for any pending load, then destroy the layer. */
    ~DepthmapLayer() override;

    /** @brief Get the depth map path. */
    QString source() const
    {
        return _source;
    }

    /**
     * @brief Start loading a new depth map in the background.
     * @param[in] path Path or file:// URL of the depthMap or simMap file.
     */
    void setSource(const QString& path);

    /** @brief Returns true while a depth map is being loaded. */
    bool loading() const
    {
        return _loading;
    }

    /** @brief Get the error message of the last failed load. */
    QString errorString() const
    {
        return _errorString;
    }

    /** @brief Returns the most recently loaded depth map mesh data. */
    const DepthmapData& depthmapData() const
    {
        return *_depthmapData;
    }

    /** @brief Returns true when depthmapData() changed since the last clearDataDirty() call. */
    bool dataDirty() const
    {
        return _dataDirty;
    }

    /** @brief Clears the dirty flag once the renderable has consumed depthmapData(). */
    void clearDataDirty()
    {
        _dataDirty = false;
    }

    /** @brief Create a DepthmapRenderable. */
    std::unique_ptr<IRenderable> createRenderable() const override
    {
        return std::make_unique<DepthmapRenderable>();
    }

    /** @brief Returns the bounds of the loaded mesh, or an empty box. */
    BoundingBox boundingBox() const override
    {
        return _depthmapData->valid ? _depthmapData->boundingBox : BoundingBox();
    }

    /** @brief The depth map is drawn after the regular layers. */
    bool rendersInForeground() const override
    {
        return true;
    }

  signals:
    /** @brief Emitted when the source path changes. */
    void sourceChanged();
    /** @brief Emitted when loading starts or ends. */
    void loadingChanged();
    /** @brief Emitted after each load completes. */
    void errorStringChanged();

  private slots:
    /** @brief Collect the background load result. */
    void onLoadFinished();

  private:
    /** @brief Depth map path. */
    QString _source;
    /** @brief Loading flag. */
    bool _loading = false;
    /** @brief Error message of the last failed load. */
    QString _errorString;
    /** @brief True when the mesh data waits for the renderable. */
    bool _dataDirty = false;
    /** @brief Last loaded mesh data. */
    std::unique_ptr<DepthmapData> _depthmapData = std::make_unique<DepthmapData>();
    /** @brief Watcher of the background load task. */
    QFutureWatcher<std::unique_ptr<DepthmapData>> _watcher;
};
