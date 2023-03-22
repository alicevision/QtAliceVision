#include "MSfMData.hpp"

#include <QDebug>
#include <QFileInfo>
#include <QThreadPool>

#include <aliceVision/sfmDataIO/sfmDataIO.hpp>

namespace qtAliceVision
{

void SfmDataIORunnable::run()
{
    using namespace aliceVision;

    // std::unique_ptr<sfmData::SfMData> sfmData(new sfmData::SfMData());

    try
    {
        if (!sfmDataIO::Load(*_sfmData, _sfmDataPath.toLocalFile().toStdString(), sfmDataIO::ESfMData::ALL))
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

    Q_EMIT resultReady(); // sfmData.release());
}

MSfMData::MSfMData()
    : _sfmData(new aliceVision::sfmData::SfMData())
    , _loadingSfmData(new aliceVision::sfmData::SfMData())
{
    connect(this, &MSfMData::sfmDataPathChanged, this, &MSfMData::load);
}

MSfMData::~MSfMData()
{
    setStatus(None);
}

void MSfMData::clear()
{
    _sfmData->clear();
    Q_EMIT sfmDataChanged();
}

void MSfMData::load()
{
    _outdated = false;

    if (_sfmDataPath.isEmpty())
    {
        if (status() == Loading)
            _outdated = true;
        setStatus(None);
        clear();
        return;
    }
    if (!QFileInfo::exists(_sfmDataPath.toLocalFile()))
    {
        setStatus(Error);
        clear();
        return;
    }
    if (!(status() == Loading))
    {
        setStatus(Loading);

        _loadingSfmData->clear();
        // load features from file in a seperate thread
        SfmDataIORunnable* ioRunnable = new SfmDataIORunnable(_sfmDataPath, _loadingSfmData.get());
        connect(ioRunnable, &SfmDataIORunnable::resultReady, this, &MSfMData::onSfmDataReady);
        QThreadPool::globalInstance()->start(ioRunnable);
    }
    else
    {
        _outdated = true;
    }
}

QString MSfMData::getUrlFromViewId(int viewId)
{
    return QString::fromUtf8(_sfmData->getView(aliceVision::IndexT(viewId)).getImagePath().c_str());
}

void MSfMData::onSfmDataReady()
{
    if (!_loadingSfmData)
        return;

    if (_outdated)
    {
        clear();
        setStatus(None);
        load();
        return;
    }

    setStatus(Loading);
    _sfmData.swap(_loadingSfmData);
    _loadingSfmData->clear();
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
        if (landmark.descType != descType) continue;

        const auto observationIt = landmark.observations.find(viewId);
        if (observationIt == landmark.observations.end()) continue;

        ++count;
    }

    return count;
}

} // namespace qtAliceVision

#include "MSfMData.moc"
