#pragma once

#include <MeshLayer/MeshData.hpp>

#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector3D>
#include <memory>
#include <qqml.h>

/** @brief Mesh file loaded asynchronously, shareable between several MeshLayer instances. */
class MeshObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Path or file:// URL of the mesh to load; an empty path clears the mesh. */
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    /** @brief True while a mesh is being loaded. */
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    /** @brief Error message of the last failed load, empty on success. */
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    /** @brief True when a mesh is loaded. */
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)
    /** @brief Mesh bounds as [min, max] QVector3D values, or an empty list. */
    Q_PROPERTY(QVariantList boundingBox READ boundingBox NOTIFY boundingBoxChanged)
    /** @brief True when the loaded mesh has at least one diffuse texture, i.e. MeshLayer.MeshMaterial shading is meaningful. */
    Q_PROPERTY(bool hasTextures READ hasTextures NOTIFY hasTexturesChanged)

  public:
    /**
     * @brief Build an empty mesh object.
     * @param[in] parent Optional Qt parent object.
     */
    explicit MeshObject(QObject* parent = nullptr);
    /** @brief Wait for any pending load, then destroy the object. */
    ~MeshObject() override;

    /** @brief Get the mesh path. */
    QString source() const
    {
        return _source;
    }

    /**
     * @brief Start loading a new mesh in the background.
     * @param[in] path Path or file:// URL of the mesh; an empty path clears the mesh.
     */
    void setSource(const QString& path);

    /** @brief Returns true while a mesh is being loaded. */
    bool loading() const
    {
        return _loading;
    }

    /** @brief Get the error message of the last failed load. */
    QString errorString() const
    {
        return _errorString;
    }

    /** @brief Returns true when a mesh is loaded. */
    bool valid() const
    {
        return _meshData && _meshData->valid;
    }

    /** @brief Returns true when the loaded mesh has at least one texture. */
    bool hasTextures() const
    {
        return valid() && !_meshData->textures.isEmpty();
    }

    /** @brief Returns the mesh bounds as [min, max], or an empty list when there is no geometry. */
    QVariantList boundingBox() const
    {
        QVariantList box;
        if (!_meshData || !_meshData->valid)
        {
            return box;
        }

        const BoundingBox& bounds = _meshData->boundingBox;
        if (!bounds.isValid())
        {
            return box;
        }

        box << QVariant::fromValue(bounds.min());
        box << QVariant::fromValue(bounds.max());

        return box;
    }

    /** @brief Returns the loaded mesh data. */
    const MeshData& meshData() const
    {
        return *_meshData;
    }

  signals:
    /** @brief Emitted when the source path changes. */
    void sourceChanged();
    /** @brief Emitted when loading starts or ends. */
    void loadingChanged();
    /** @brief Emitted after each load completes. */
    void errorStringChanged();
    /** @brief Emitted when the validity changes. */
    void validChanged();
    /** @brief Emitted when the bounds may have changed. */
    void boundingBoxChanged();
    /** @brief Emitted when texture availability may have changed. */
    void hasTexturesChanged();
    /** @brief Emitted when a mesh was loaded successfully. */
    void dataReady();

  private slots:
    /** @brief Collect the background load result and notify. */
    void onLoadFinished();

  private:
    /** @brief Mesh path. */
    QString _source;
    /** @brief Loading flag. */
    bool _loading = false;
    /** @brief Error message of the last failed load. */
    QString _errorString;
    /** @brief Last loaded mesh data. */
    std::unique_ptr<MeshData> _meshData = std::make_unique<MeshData>();
    /** @brief Watcher of the background load task. */
    QFutureWatcher<std::unique_ptr<MeshData>> _watcher;
};
