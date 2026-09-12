#include <MeshLayer/MeshObject.hpp>
#include <MeshLayer/MeshLoader.hpp>

#include <QtConcurrent/QtConcurrent>
#include <QUrl>

MeshObject::MeshObject(QObject* parent)
  : QObject(parent)
{
    connect(&_watcher, &QFutureWatcher<std::unique_ptr<MeshData>>::finished, this, &MeshObject::onLoadFinished);
}

MeshObject::~MeshObject()
{
    if (_watcher.isRunning())
        _watcher.waitForFinished();
}

void MeshObject::setSource(const QString& path)
{
    if (_source == path)
        return;

    _source = path;
    emit sourceChanged();

    if (path.isEmpty())
    {
        _meshData = std::make_unique<MeshData>();
        emit validChanged();
        emit boundingBoxChanged();
        return;
    }

    QString filePath = path;
    if (filePath.startsWith("file://"))
        filePath = QUrl(filePath).toLocalFile();

    _loading = true;
    emit loadingChanged();

    _watcher.setFuture(QtConcurrent::run([filePath]() { return MeshLoader::load(filePath); }));
}

void MeshObject::onLoadFinished()
{
    const bool wasValid = valid();

    _meshData = _watcher.future().takeResult();
    _loading = false;

    if (!_meshData->valid)
    {
        _errorString = _meshData->errorString;
    }
    else
    {
        _errorString.clear();
    }

    emit loadingChanged();
    emit errorStringChanged();

    if (valid() != wasValid)
    {
        emit validChanged();
    }

    emit boundingBoxChanged();

    if (_meshData->valid)
    {
        emit dataReady();
    }
}
