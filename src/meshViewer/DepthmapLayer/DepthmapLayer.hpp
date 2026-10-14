#pragma once

#include <Core/LayerItem.hpp>
#include <DepthmapLayer/DepthmapRenderable.hpp>
#include <DepthmapLayer/DepthmapData.hpp>
#include <QFutureWatcher>

class DepthmapLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

  public:
    explicit DepthmapLayer(QObject* parent = nullptr);
    ~DepthmapLayer() override;

    QString source() const
    {
        return _source;
    }

    void setSource(const QString& path);

    bool loading() const
    {
        return _loading;
    }

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

    std::unique_ptr<IRenderable> createRenderable() const override
    {
        return std::make_unique<DepthmapRenderable>();
    }

    BoundingBox boundingBox() const override
    {
        return _depthmapData->valid ? _depthmapData->boundingBox : BoundingBox();
    }

    bool rendersInForeground() const override
    {
        return true;
    }

  signals:
    void sourceChanged();
    void loadingChanged();
    void errorStringChanged();

  private slots:
    void onLoadFinished();

  private:
    QString _source;
    bool _loading = false;
    QString _errorString;
    bool _dataDirty = false;
    std::unique_ptr<DepthmapData> _depthmapData = std::make_unique<DepthmapData>();
    QFutureWatcher<std::unique_ptr<DepthmapData>> _watcher;
};
