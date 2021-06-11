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
     * @brief Load and display image.
     */
    class PanoramaViewer : public QQuickItem
    {
        Q_OBJECT
            /// Path to image
            Q_PROPERTY(QSize textureSize MEMBER _textureSize NOTIFY textureSizeChanged)
            ///
            Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)

            Q_PROPERTY(bool clearBeforeLoad MEMBER _clearBeforeLoad NOTIFY clearBeforeLoadChanged)

            Q_PROPERTY(QVariantMap imagesData READ imagesData NOTIFY imagesDataChanged)

    public:
        explicit PanoramaViewer(QQuickItem* parent = nullptr);
        ~PanoramaViewer() override;

        QSize sourceSize() const
        {
            return _sourceSize;
        }

        const QVariantMap& metadata() const
        {
            return _metadata;
        }

        const QVariantMap& imagesData() const
        {
            return _imagesData;
        }

    public:
        Q_SIGNAL void clearBeforeLoadChanged();
        Q_SIGNAL void textureSizeChanged();
        Q_SIGNAL void sourceSizeChanged();
        Q_SIGNAL void imageChanged();
        Q_SIGNAL void metadataChanged();
        Q_SIGNAL void sfmChanged();
        Q_SIGNAL void imagesDataChanged(const QVariantMap& imagesData);
        
        Q_INVOKABLE void setSfmPath(const QString& path);
        Q_INVOKABLE int getDownscale() const;

    private:
        /// Custom QSGNode update
        QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;
        void computeInputImages();

    private:
        bool _clearBeforeLoad = true;

        QSize _textureSize;
        QSize _sourceSize = QSize(0, 0);

        QVariantMap _metadata;

        QString _sfmPath;
        QVariantMap _imagesData;

        int _downscale;
    };

}  // ns qtAliceVision

Q_DECLARE_METATYPE(QList<QPoint>)


