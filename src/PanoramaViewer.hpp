#pragma once

#include "FloatTexture.hpp"
#include "FloatImageViewer.hpp"
#include "Surface.hpp"

#include <QQuickItem>
#include <QUrl>
#include <QRunnable>
#include <QSharedPointer>
#include <QVariant>
#include <QVector4D>

#include <aliceVision/numeric/numeric.hpp>
#include <aliceVision/types.hpp>

#include <map>
#include <string>
#include <memory>

namespace qtAliceVision
{
    /**
     * @brief Displays a list of Float Images.
     */
    class PanoramaViewer : public QQuickItem
    {
        Q_OBJECT
            Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)

            Q_PROPERTY(QVariantMap imagesData READ imagesData NOTIFY imagesDataChanged)

            Q_PROPERTY(QString sfmPath WRITE setSfmPath NOTIFY sfmPathChanged)

            Q_PROPERTY(int downscale MEMBER _downscale NOTIFY downscaleChanged)

    public:
        explicit PanoramaViewer(QQuickItem* parent = nullptr);
        ~PanoramaViewer() override;

        QSize sourceSize() const
        {
            return _sourceSize;
        }

        const QVariantMap& imagesData() const
        {
            return _imagesData;
        }

        void setSfmPath(const QString& path);

    public:
        Q_SIGNAL void sourceSizeChanged();

        Q_SIGNAL void sfmPathChanged();

        Q_SIGNAL void downscaleChanged();

        Q_SIGNAL void imagesDataChanged(const QVariantMap& imagesData);
        
    private:
        /// Custom QSGNode update
        QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;

        void computeInputImages();

    private:
        QSize _sourceSize = QSize(3000, 1500);
        
        QString _sfmPath;

        QVariantMap _imagesData;

        int _downscale = 0;
    };

}  // ns qtAliceVision

Q_DECLARE_METATYPE(QList<QPoint>)


