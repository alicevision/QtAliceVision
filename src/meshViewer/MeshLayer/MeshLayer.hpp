#pragma once

#include <Core/LayerItem.hpp>
#include <MeshLayer/MeshObject.hpp>
#include <MeshLayer/MeshPicker.hpp>

#include <QVector3D>

class MeshLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(MeshObject* mesh READ mesh WRITE setMesh NOTIFY meshChanged)
    Q_PROPERTY(QVector3D selection READ selection WRITE setSelection NOTIFY selectionChanged)
    Q_PROPERTY(WireframeMode wireframeMode READ wireframeMode WRITE setWireframeMode NOTIFY wireframeModeChanged)
    Q_PROPERTY(ShadingMode shadingMode READ shadingMode WRITE setShadingMode NOTIFY shadingModeChanged)

  public:
    enum WireframeMode
    {
        Solid = 0,
        Overlay = 1,
        WireframeOnly = 2
    };
    Q_ENUM(WireframeMode)

    enum ShadingMode
    {
        MeshShaded = 0,
        MeshNormal = 1
    };
    Q_ENUM(ShadingMode)

    explicit MeshLayer(QObject* parent = nullptr);

    MeshObject* mesh() const
    {
        return _mesh;
    }
    void setMesh(MeshObject* mesh);

    QVector3D selection() const
    {
        return _selection;
    }
    void setSelection(const QVector3D& selection);

    WireframeMode wireframeMode() const
    {
        return _wireframeMode;
    }
    void setWireframeMode(WireframeMode mode);

    ShadingMode shadingMode() const
    {
        return _shadingMode;
    }
    void setShadingMode(ShadingMode mode);

    const MeshData& meshData() const
    {
        return _mesh->meshData();
    }
    bool meshDirty() const
    {
        return _meshDirty;
    }
    void clearMeshDirty()
    {
        _meshDirty = false;
    }
    bool selectionDirty() const
    {
        return _selectionDirty;
    }
    void clearSelectionDirty()
    {
        _selectionDirty = false;
    }
    bool wireframeDirty() const
    {
        return _wireframeDirty;
    }
    void clearWireframeDirty()
    {
        _wireframeDirty = false;
    }
    bool shadingDirty() const
    {
        return _shadingDirty;
    }
    void clearShadingDirty()
    {
        _shadingDirty = false;
    }
    bool opacityDirty() const
    {
        return _opacityDirty;
    }
    void clearOpacityDirty()
    {
        _opacityDirty = false;
    }

    bool canPick() const override
    {
        return _mesh && _mesh->valid();
    }
    LayerPickResult pick(const Ray& ray) const override;
    void applyPickResult(const LayerPickResult& result) override;
    void clearPick() override;

  signals:
    void meshChanged();
    void selectionChanged();
    void wireframeModeChanged();
    void shadingModeChanged();

  public:
    std::unique_ptr<IRenderable> createRenderable() const override;

  private:
    void onMeshDataReady();

    MeshObject* _mesh = nullptr;
    QMetaObject::Connection _meshConnection;
    QVector3D _selection;
    bool _meshDirty = false;
    bool _selectionDirty = true;
    WireframeMode _wireframeMode = Solid;
    bool _wireframeDirty = false;
    ShadingMode _shadingMode = MeshShaded;
    bool _shadingDirty = false;
    bool _opacityDirty = true;
    MeshPicker _picker;
};
