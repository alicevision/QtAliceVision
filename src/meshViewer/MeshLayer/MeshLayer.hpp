#pragma once

#include <Core/LayerItem.hpp>
#include <MeshLayer/MeshObject.hpp>
#include <MeshLayer/MeshPicker.hpp>

#include <QVector3D>

/** @brief Layer drawing a MeshObject, with configurable shading and wireframe, and Embree-based picking. */
class MeshLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Mesh to draw. The layer does not take ownership. */
    Q_PROPERTY(MeshObject* mesh READ mesh WRITE setMesh NOTIFY meshChanged)
    /** @brief World-space point selected by the last successful pick. */
    Q_PROPERTY(QVector3D selection READ selection WRITE setSelection NOTIFY selectionChanged)
    /** @brief How triangle edges are displayed. */
    Q_PROPERTY(WireframeMode wireframeMode READ wireframeMode WRITE setWireframeMode NOTIFY wireframeModeChanged)
    /** @brief How the mesh surface is colored. */
    Q_PROPERTY(ShadingMode shadingMode READ shadingMode WRITE setShadingMode NOTIFY shadingModeChanged)

  public:
    /** @brief How triangle edges are displayed. */
    enum WireframeMode
    {
        Solid = 0,        /**< Surface only. */
        Overlay = 1,      /**< Surface with edges drawn on top. */
        WireframeOnly = 2 /**< Edges only; hidden edges are still occluded by a depth-only pass. */
    };
    Q_ENUM(WireframeMode)

    /** @brief How the solid mesh surface is colored. */
    enum ShadingMode
    {
        MeshShaded = 0,  /**< Constant color with simple diffuse lighting. */
        MeshNormal = 1,  /**< Surface normal displayed as color. */
        MeshMaterial = 2 /**< Unlit material: diffuse texture times base color; falls back to MeshShaded when the mesh has no texture. */
    };
    Q_ENUM(ShadingMode)

    /**
     * @brief Build a layer without mesh.
     * @param[in] parent Optional Qt parent object.
     */
    explicit MeshLayer(QObject* parent = nullptr);

    /** @brief Get the drawn mesh. */
    MeshObject* mesh() const
    {
        return _mesh;
    }
    /**
     * @brief Set the mesh to draw and follow its data updates.
     * @param[in] mesh Mesh object, or nullptr.
     */
    void setMesh(MeshObject* mesh);

    /** @brief Get the selected world-space point. */
    QVector3D selection() const
    {
        return _selection;
    }
    /**
     * @brief Set the selected world-space point.
     * @param[in] selection Point, in world coordinates.
     */
    void setSelection(const QVector3D& selection);

    /** @brief Get the wireframe mode. */
    WireframeMode wireframeMode() const
    {
        return _wireframeMode;
    }
    /**
     * @brief Set the wireframe mode.
     * @param[in] mode New mode.
     */
    void setWireframeMode(WireframeMode mode);

    /** @brief Get the shading mode. */
    ShadingMode shadingMode() const
    {
        return _shadingMode;
    }
    /**
     * @brief Set the shading mode.
     * @param[in] mode New mode.
     */
    void setShadingMode(ShadingMode mode);

    /**
     * @brief Returns the data of the drawn mesh.
     * @warning The mesh must be set.
     */
    const MeshData& meshData() const
    {
        return _mesh->meshData();
    }
    /** @brief Returns true when the mesh data changed since the renderable last consumed it. */
    bool meshDirty() const
    {
        return _meshDirty;
    }
    /** @brief Mark the mesh data as consumed by the renderable. */
    void clearMeshDirty()
    {
        _meshDirty = false;
    }
    /** @brief Returns true when the selection changed since the renderable last consumed it. */
    bool selectionDirty() const
    {
        return _selectionDirty;
    }
    /** @brief Mark the selection as consumed by the renderable. */
    void clearSelectionDirty()
    {
        _selectionDirty = false;
    }
    /** @brief Returns true when the wireframe mode changed since the renderable last consumed it. */
    bool wireframeDirty() const
    {
        return _wireframeDirty;
    }
    /** @brief Mark the wireframe mode as consumed by the renderable. */
    void clearWireframeDirty()
    {
        _wireframeDirty = false;
    }
    /** @brief Returns true when the shading mode changed since the renderable last consumed it. */
    bool shadingDirty() const
    {
        return _shadingDirty;
    }
    /** @brief Mark the shading mode as consumed by the renderable. */
    void clearShadingDirty()
    {
        _shadingDirty = false;
    }
    /** @brief Returns true when the opacity changed since the renderable last consumed it. */
    bool opacityDirty() const
    {
        return _opacityDirty;
    }
    /** @brief Mark the opacity as consumed by the renderable. */
    void clearOpacityDirty()
    {
        _opacityDirty = false;
    }

    /** @brief Returns the bounds of the mesh, or an empty box when no valid mesh is set. */
    BoundingBox boundingBox() const override
    {
        if (!_mesh || !_mesh->valid())
        {
            return {};
        }
        return _mesh->meshData().boundingBox;
    }

    /** @brief Returns true when a valid mesh is set. */
    bool canPick() const override
    {
        return _mesh && _mesh->valid();
    }
    /**
     * @brief Intersect the ray with the mesh triangles.
     * @param[in] ray World-space ray.
     * @return The closest hit, or no hit when picking is disabled or the BVH is not ready.
     */
    LayerPickResult pick(const Ray& ray) const override;
    /**
     * @brief Store the hit point as the selection.
     * @param[in] result Pick result.
     */
    void applyPickResult(const LayerPickResult& result) override;
    /** @brief Keep the current selection when another layer is picked. */
    void clearPick() override;

  signals:
    /** @brief Emitted when the mesh object is replaced. */
    void meshChanged();
    /** @brief Emitted when the selection changes. */
    void selectionChanged();
    /** @brief Emitted when the wireframe mode changes. */
    void wireframeModeChanged();
    /** @brief Emitted when the shading mode changes. */
    void shadingModeChanged();

  public:
    /** @brief Create a MeshRenderable. */
    std::unique_ptr<IRenderable> createRenderable() const override;

  private:
    /** @brief Flag everything dirty and rebuild the picking BVH after the mesh data changed. */
    void onMeshDataReady();

    /** @brief Drawn mesh, not owned. */
    MeshObject* _mesh = nullptr;
    /** @brief Connection to MeshObject::dataReady. */
    QMetaObject::Connection _meshConnection;
    /** @brief Selected world-space point. */
    QVector3D _selection;
    /** @brief Mesh data dirty flag. */
    bool _meshDirty = false;
    /** @brief Selection dirty flag. */
    bool _selectionDirty = true;
    /** @brief Wireframe mode. */
    WireframeMode _wireframeMode = Solid;
    /** @brief Wireframe mode dirty flag. */
    bool _wireframeDirty = false;
    /** @brief Shading mode. */
    ShadingMode _shadingMode = MeshShaded;
    /** @brief Shading mode dirty flag. */
    bool _shadingDirty = false;
    /** @brief Opacity dirty flag. */
    bool _opacityDirty = true;
    /** @brief Ray-casting acceleration structure. */
    MeshPicker _picker;
};
