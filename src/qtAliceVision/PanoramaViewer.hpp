#pragma once

#include "FloatImageViewer.hpp"
#include "FloatTexture.hpp"
#include "Surface.hpp"

#include <QQuickItem>
#include <QRunnable>
#include <QSharedPointer>
#include <QUrl>
#include <QVariant>
#include <QVector4D>

#include <aliceVision/numeric/numeric.hpp>
#include <aliceVision/types.hpp>

#include <map>
#include <memory>
#include <string>

namespace qtAliceVision
{
/**
 * @brief Displays a list of Float Images.
 */
class PanoramaViewer : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)

    Q_PROPERTY(qtAliceVision::MSfMData* msfmData READ getMSfmData WRITE setMSfmData NOTIFY sfmDataChanged)

    Q_PROPERTY(int downscale MEMBER _downscale NOTIFY downscaleChanged)

public:
    explicit PanoramaViewer(QQuickItem* parent = nullptr);
    ~PanoramaViewer() override;

    QSize sourceSize() const { return _sourceSize; }

    MSfMData* getMSfmData() { return _msfmData; }
    void setMSfmData(MSfMData* sfmData);

    void msfmDataUpdate() { computeDownscale(); }

public:
    Q_SIGNAL void sourceSizeChanged();

    Q_SIGNAL void sfmDataChanged();

    Q_SIGNAL void downscaleChanged();

    Q_SIGNAL void downscaleReady();

private:
    /// Custom QSGNode update
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;

    void computeDownscale();

private:
    QSize _sourceSize = QSize(3000, 1500);

    MSfMData* _msfmData = nullptr;

    int _downscale = 4;
};

} // namespace qtAliceVision

Q_DECLARE_METATYPE(QList<QPoint>)
