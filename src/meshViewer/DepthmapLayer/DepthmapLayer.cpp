#include <DepthmapLayer/DepthmapLayer.hpp>
#include <DepthmapLayer/DepthmapLoader.hpp>

#include <QtConcurrent/QtConcurrent>
#include <QUrl>

DepthmapLayer::DepthmapLayer(QObject* parent)
  : LayerItem(parent)
{
    connect(&_watcher, &QFutureWatcher<std::unique_ptr<DepthmapData>>::finished, this, &DepthmapLayer::onLoadFinished);
}

DepthmapLayer::~DepthmapLayer()
{
    if (_watcher.isRunning())
    {
        _watcher.waitForFinished();
    }
}

void DepthmapLayer::setSource(const QString& path)
{
    if (_source == path)
    {
        return;
    }

    _source = path;
    emit sourceChanged();

    if (path.isEmpty())
    {
        _depthmapData = std::make_unique<DepthmapData>();
        _dataDirty = true;
        emit dataReady();
        return;
    }

    QString filePath = path;
    if (filePath.startsWith("file://"))
    {
        filePath = QUrl(filePath).toLocalFile();
    }

    _loading = true;
    emit loadingChanged();

    _watcher.setFuture(QtConcurrent::run([filePath]() { return DepthmapLoader::load(filePath); }));
}

void DepthmapLayer::onLoadFinished()
{
    _depthmapData = _watcher.future().takeResult();
    _loading = false;

    if (!_depthmapData->valid)
    {
        _errorString = _depthmapData->errorString;
    }
    else
    {
        _errorString.clear();
        _dataDirty = true;
        emit dataReady();
    }

    emit loadingChanged();
    emit errorStringChanged();
}
