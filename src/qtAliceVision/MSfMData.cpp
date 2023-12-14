#include "MSfMData.hpp"

#include <QDebug>
#include <QFileInfo>
#include <QThreadPool>

#include <aliceVision/sfmDataIO/sfmDataIO.hpp>

namespace qtAliceVision {

void SfmDataIORunnable::run()
{
    aliceVision::sfmData::SfMData* sfmData = new aliceVision::sfmData::SfMData;

    try
    {
        if (!aliceVision::sfmDataIO::Load(*sfmData, _sfmDataPath.toLocalFile().toStdString(), aliceVision::sfmDataIO::ESfMData::ALL))
        {
            qDebug() << "[QtAliceVision] Failed to load sfmData: " << _sfmDataPath << ".";
        }
    }
    catch (std::exception& e)
    {
        qDebug() << "[QtAliceVision] Failed to load sfmData: " << _sfmDataPath << "."
                 << "\n"
                 << e.what();
    }

    Q_EMIT resultReady(sfmData);
}

MSfMData::MSfMData() { connect(this, &MSfMData::sfmDataPathChanged, this, &MSfMData::load); }

MSfMData::~MSfMData()
{
    if (_sfmData)
        delete _sfmData;

    setStatus(None);
}

void MSfMData::load()
{
    _needReload = false;

    if (_status == Loading)
    {
        qDebug("[QtAliceVision] SfMData: Unable to load, a load event is already running.");
        _needReload = true;
        return;
    }

    if (_sfmDataPath.isEmpty())
    {
        setStatus(None);
        return;
    }
    if (!QFileInfo::exists(_sfmDataPath.toLocalFile()))
    {
        setStatus(Error);
        return;
    }

    setStatus(Loading);

    // load features from file in a seperate thread
    SfmDataIORunnable* ioRunnable = new SfmDataIORunnable(_sfmDataPath);
    connect(ioRunnable, &SfmDataIORunnable::resultReady, this, &MSfMData::onSfmDataReady);
    QThreadPool::globalInstance()->start(ioRunnable);
}

QString MSfMData::getUrlFromViewId(int viewId)
{
    return QString::fromUtf8(_sfmData->getView(aliceVision::IndexT(viewId)).getImage().getImagePath().c_str());
}

void MSfMData::onSfmDataReady(aliceVision::sfmData::SfMData* sfmData)
{
    if (_needReload)
    {
        if (sfmData)
            delete sfmData;

        setStatus(None);
        load();
        return;
    }

    if (sfmData)
    {
        if (_sfmData)
            delete _sfmData;

        _sfmData = sfmData;
    }

    setStatus(Ready);
}

void MSfMData::setStatus(Status status)
{
    if (status == _status)
        return;
    _status = status;
    Q_EMIT statusChanged(_status);
    if (status == Ready || status == Error)
    {
        Q_EMIT sfmDataChanged();
    }
}

size_t MSfMData::nbCameras() const
{
    if (!_sfmData || _status != Ready)
        return 0;
    return _sfmData->getValidViews().size();
}

QVariantList MSfMData::getViewsIds() const
{
    QVariantList viewsIds;
    for (const auto& id : _sfmData->getValidViews())
    {
        viewsIds.append(id);
    }
    return viewsIds;
}

int MSfMData::nbLandmarks(QString describerType, int viewId) const
{
    if (_status != Ready)
    {
        return 0;
    }

    if (!_sfmData)
    {
        return 0;
    }

    const auto& landmarks = _sfmData->getLandmarks();

    int count = 0;
    auto descType = aliceVision::feature::EImageDescriberType_stringToEnum(describerType.toStdString());
    for (const auto& [_, landmark] : landmarks)
    {
        if (landmark.descType != descType)
            continue;

        const auto observationIt = landmark.getObservations().find(viewId);
        if (observationIt == landmark.getObservations().end())
            continue;

        ++count;
    }

    return count;
}

}  // namespace qtAliceVision

#include "MSfMData.moc"
