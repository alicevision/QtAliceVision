#include "IOThread.hpp"
#include <QFile>

namespace abcentity
{

void IOThread::read(const QUrl& source)
{
    _source = source;
    start();
}

void IOThread::run()
{
    // ensure file exists and is valid
    if(!_source.isValid() || !QFile::exists(_source.toLocalFile()))
        return;
    QMutexLocker lock(&_mutex);
    _archive = _factory.getArchive(_source.toLocalFile().toStdString(), _coreType);
}

void IOThread::clear()
{
    QMutexLocker lock(&_mutex);
    _archive.reset();
}

const Alembic::Abc::IArchive& IOThread::archive() const
{    
    // mutex is mutable and can be locked in const methods
    QMutexLocker lock(&_mutex);
    return _archive;
}

}