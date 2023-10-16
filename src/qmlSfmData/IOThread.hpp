#pragma once

#include <QThread>
#include <QUrl>
#include <QMutex>

#include <aliceVision/sfmDataIO/sfmDataIO.hpp>


namespace sfmdataentity
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

    /// Get the sfmData
    const aliceVision::sfmData::SfMData & getSfmData() const;

private:
    QUrl _source;
    mutable QMutex _mutex;
    aliceVision::sfmData::SfMData _sfmData;
};

}  // namespace sfmdataentity
