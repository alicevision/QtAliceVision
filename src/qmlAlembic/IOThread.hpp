#pragma once

#include <QThread>
#include <QUrl>
#include <QMutex>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreFactory/All.h>

namespace abcentity
{

/**
 * @brief Handle Alembic IO in a separate thread.
 */
class IOThread : public QThread
{
    Q_OBJECT

public:
    /// Read the given source. Starts the thread main loop.
    void read(const QUrl& source);
    /// Thread main loop.
    void run() override;
    /// Reset internal members.
    void clear();
    /// Get the Alembic archive.
    const Alembic::Abc::IArchive& archive() const;

private:
    QUrl _source;
    mutable QMutex _mutex;
    Alembic::AbcCoreFactory::IFactory _factory;
    Alembic::AbcCoreFactory::IFactory::CoreType _coreType;
    Alembic::Abc::IArchive _archive;
};

}