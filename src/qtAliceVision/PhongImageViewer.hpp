#pragma once

#include "FloatTexture.hpp"
#include "SingleImageLoader.hpp"

#include <QQuickItem>
#include <QSGGeometryNode>
#include <QVariant>

#include <memory>

namespace qtAliceVision {

/**
 * @brief Load and display image (albedo and normal) with a given light direction.
 *        Shading is done using Blinn-Phong reflection model.
 */
class PhongImageViewer : public QQuickItem
{
    Q_OBJECT

    // file paths
    Q_PROPERTY(QUrl sourcePath MEMBER _sourcePath NOTIFY sourcePathChanged)
    Q_PROPERTY(QUrl normalPath MEMBER _normalPath NOTIFY normalPathChanged)

    // source parameters (gamma, gain, channelMode)
    Q_PROPERTY(EChannelMode channelMode MEMBER _channelMode NOTIFY sourceParametersChanged)
    Q_PROPERTY(float gamma MEMBER _gamma NOTIFY sourceParametersChanged)
    Q_PROPERTY(float gain MEMBER _gain NOTIFY sourceParametersChanged)
    
    // shading parameters (material, light)
    Q_PROPERTY(QColor baseColor MEMBER _baseColor NOTIFY shadingParametersChanged)
    Q_PROPERTY(float textureOpacity MEMBER _textureOpacity NOTIFY shadingParametersChanged)
    Q_PROPERTY(float ka MEMBER _ka NOTIFY shadingParametersChanged)
    Q_PROPERTY(float kd MEMBER _kd NOTIFY shadingParametersChanged)
    Q_PROPERTY(float ks MEMBER _ks NOTIFY shadingParametersChanged)
    Q_PROPERTY(float shininess MEMBER _shininess NOTIFY shadingParametersChanged)
    Q_PROPERTY(float lightYaw MEMBER _lightYaw NOTIFY shadingParametersChanged)
    Q_PROPERTY(float lightPitch MEMBER _lightPitch NOTIFY shadingParametersChanged)

    // texture
    Q_PROPERTY(QSize textureSize MEMBER _textureSize NOTIFY textureSizeChanged)
    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)
    
    // metadata
    Q_PROPERTY(QVariantMap metadata READ metadata NOTIFY metadataChanged)

    // viewer status
    Q_PROPERTY(EStatus status READ status NOTIFY statusChanged)

  public:

    enum class EStatus : quint8
    {
        NONE,              // nothing is happening, no error has been detected
        LOADING,           // an image is being loaded
        MISSING_FILE,      // the file to load is missing
        LOADING_ERROR,     // generic error
    };
    Q_ENUM(EStatus)

    enum class EChannelMode : quint8
    {
        RGBA,
        RGB,
        R,
        G,
        B,
        A
    };
    Q_ENUM(EChannelMode)

    // constructor
    explicit PhongImageViewer(QQuickItem* parent = nullptr);

    // destructor
    ~PhongImageViewer() override;

    // accessors
    EStatus status() const { return _status; }
    const QSize& sourceSize() const { return _sourceSize; }
    const QVariantMap& metadata() const { return _metadata; }

    // signals
    Q_SIGNAL void sourcePathChanged();
    Q_SIGNAL void normalPathChanged();  
    Q_SIGNAL void sourceParametersChanged();
    Q_SIGNAL void shadingParametersChanged();
    Q_SIGNAL void textureSizeChanged();
    Q_SIGNAL void sourceSizeChanged();
    Q_SIGNAL void metadataChanged();
    Q_SIGNAL void imageChanged();
    Q_SIGNAL void statusChanged();

  private:

    // set viewer status
    void setStatus(EStatus status);

    // reload image(s) 
    void reload();

    // clear images in memory
    void clearImages();

    // custom QSGNode update
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data) override;

  private:

    // file paths
    QUrl _sourcePath;
    QUrl _normalPath;

    // source parameters (gamma, gain, channelMode)
    EChannelMode _channelMode = EChannelMode::RGBA;
    float _gamma = 1.f;
    float _gain = 1.f;
    bool _sourceParametersChanged = false;

    // shading parameters (material, light)
    QColor _baseColor = {50, 50, 50};
    float _textureOpacity = 1.f;
    float _ka = 0.f;
    float _kd = 1.f;
    float _ks = 1.f;
    float _shininess = 20.f;
    float _lightYaw = 0.f;
    float _lightPitch = 0.f;
    bool _shadingParametersChanged = false;

    // metadata
    QVariantMap _metadata;

    // screen and texture size
    QRectF _boundingRect = QRectF();
    QSize _textureSize = QSize(0, 0);
    QSize _sourceSize = QSize(0, 0);

    // image
    std::shared_ptr<FloatImage> _sourceImage;
    std::shared_ptr<FloatImage> _normalImage;

    // image loaders
    imgserve::SingleImageLoader _sourceImageLoader;
    imgserve::SingleImageLoader _normalImageLoader;

    // viewer status
    EStatus _status = EStatus::NONE;
    bool _geometryChanged = false;
    bool _imageChanged = false;
};

}  // namespace qtAliceVision

