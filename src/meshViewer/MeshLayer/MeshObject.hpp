#pragma once

#include <MeshLayer/MeshData.hpp>

#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector3D>
#include <memory>
#include <qqml.h>

class MeshObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)
    Q_PROPERTY(QVariantList boundingBox READ boundingBox NOTIFY boundingBoxChanged)

  public:
    explicit MeshObject(QObject* parent = nullptr);
    ~MeshObject() override;

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
    bool valid() const
    {
        return _meshData && _meshData->valid;
    }
    QVariantList boundingBox() const
    {
        QVariantList box;
        if (!_meshData || !_meshData->valid)
            return box;

        box << QVariant::fromValue(QVector3D(_meshData->minX, _meshData->minY, _meshData->minZ));
        box << QVariant::fromValue(QVector3D(_meshData->maxX, _meshData->maxY, _meshData->maxZ));
        return box;
    }

    const MeshData& meshData() const
    {
        return *_meshData;
    }

  signals:
    void sourceChanged();
    void loadingChanged();
    void errorStringChanged();
    void validChanged();
    void boundingBoxChanged();
    void dataReady();

  private slots:
    void onLoadFinished();

  private:
    QString _source;
    bool _loading = false;
    QString _errorString;
    std::unique_ptr<MeshData> _meshData = std::make_unique<MeshData>();
    QFutureWatcher<std::unique_ptr<MeshData>> _watcher;
};
