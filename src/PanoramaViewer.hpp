#pragma once

#include "FloatTexture.hpp"

#include <QQuickItem>
#include <QUrl>
#include <QRunnable>
#include <QSharedPointer>
#include <QVariant>
#include <QVector4D>


#include <memory>


namespace qtAliceVision
{

/**
 * @brief Load and display panorama.
 */
class PanoramaViewer : public QQuickItem
{
    Q_OBJECT
    /// Path to image
    Q_PROPERTY(QUrl source MEMBER _source NOTIFY sourceChanged)
    /// 
    Q_PROPERTY(float gamma MEMBER _gamma NOTIFY gammaChanged)
    /// 
    Q_PROPERTY(float gain MEMBER _gain NOTIFY gainChanged)
    ///
    Q_PROPERTY(QSize textureSize MEMBER _textureSize NOTIFY textureSizeChanged)
    ///
    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)
    /// Whether the image is currently being loaded from file
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    /// Whether to clear image between two loadings
    Q_PROPERTY(bool clearBeforeLoad MEMBER _clearBeforeLoad NOTIFY clearBeforeLoadChanged)

    Q_PROPERTY(EChannelMode channelMode MEMBER _channelMode NOTIFY channelModeChanged)
    
    Q_PROPERTY(QVariantMap metadata READ metadata NOTIFY metadataChanged)
    
    Q_PROPERTY(QList<QPoint> vertices READ vertices WRITE setVertices NOTIFY verticesChanged)

    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY gridColorChanged)



public:
    explicit PanoramaViewer(QQuickItem* parent = nullptr);
    ~PanoramaViewer() override;

    bool loading() const
    {
        return _loading;
    }

    void setLoading(bool loading);

    QSize sourceSize() const
    {
        return _sourceSize;
    }

    const QVariantMap & metadata() const
    {
        return _metadata;
    }

    QList<QPoint> vertices() const { return _vertices; }
    void setVertices(const QList<QPoint>& v)
    {
        
    }

    QColor gridColor() const { return _gridColor; }
    void setGridColor(const QColor & color) 
    { 
        _gridColor = color; 
        Q_EMIT gridColorChanged();
    }

    enum class EChannelMode : quint8 { RGBA, RGB, R, G, B, A };
    Q_ENUM(EChannelMode)

public:
    Q_SIGNAL void sourceChanged();
    Q_SIGNAL void loadingChanged();
    Q_SIGNAL void clearBeforeLoadChanged();
    Q_SIGNAL void gammaChanged();
    Q_SIGNAL void gainChanged();
    Q_SIGNAL void textureSizeChanged();
    Q_SIGNAL void sourceSizeChanged();
    Q_SIGNAL void channelModeChanged();
    Q_SIGNAL void imageChanged();
    Q_SIGNAL void metadataChanged();
    Q_SIGNAL void verticesChanged(bool reinit);
    Q_SIGNAL void gridColorChanged();

    Q_INVOKABLE QVector4D pixelValueAt(int x, int y);

    Q_INVOKABLE QPoint getVertex(int index) { return _vertices[index]; }
    Q_INVOKABLE void setVertex(int index, float x, float y) 
    {
        QPoint point(x, y);
        _vertices[index] = point; 
        _verticesChanged = true;
        Q_EMIT verticesChanged(false);
    }
    Q_INVOKABLE void displayGrid() 
    {
        if (_isGridDisplayed)
            _isGridDisplayed = false;
        else
            _isGridDisplayed = true;
    }
    Q_INVOKABLE void setGridColorQML(const QColor & color)
    {
        _gridColor = color;
        Q_EMIT gridColorChanged();
    }
    Q_INVOKABLE void randomControlPoints()
    {
        _randomCP = true;
        _verticesChanged = true;
        Q_EMIT verticesChanged(false);
    }
    Q_INVOKABLE void defaultControlPoints()
    {
        _vertices.clear();
        _reinit = true;
        _verticesChanged = true;
    }
    Q_INVOKABLE void resized()
    {
        _verticesChanged = true;
        Q_EMIT verticesChanged(false);
    }
    Q_INVOKABLE bool reinit() { return _reinit; }

    
private:
    /// Reload image from source
    void reload();
    /// Handle result from asynchronous file loading
    Q_SLOT void onResultReady(QSharedPointer<qtAliceVision::FloatImage> image, QSize sourceSize, const QVariantMap & metadata);
    /// Custom QSGNode update
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;

    void updatePaintGrid(QSGNode* oldNode, QSGNode* node);

private:
    QUrl _source;
    float _gamma = 1.f;
    float _gain = 1.f;

    bool _loading = false;
    bool _outdated = false;
    bool _clearBeforeLoad = true;

    bool _imageChanged = false;
    EChannelMode _channelMode;
    QSharedPointer<FloatImage> _image;
    QRectF _boundingRect;
    QSize _textureSize;
    QSize _sourceSize = QSize(0,0);

    QVariantMap _metadata;

    QList<QPoint> _vertices;
    bool _isGridDisplayed = false;
    QColor _gridColor = QColor(255, 0, 0, 255);
    bool _randomCP = false;
    bool _verticesChanged = true;
    bool _reinit = false;
};

}  // ns qtAliceVision

Q_DECLARE_METATYPE(QList<QPoint>)

