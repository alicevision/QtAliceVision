#include "FloatImageViewerMaterial.hpp"
#include "FloatImageViewerMaterialShader.hpp"

#include <functional>

namespace qtAliceVision {

FloatImageViewerMaterial::FloatImageViewerMaterial()
{
    // Initialise the texture with a 1x1 placeholder so the sampler is never unbound.
    auto image = std::make_shared<FloatImage>(1, 1, true);
    texture = std::make_unique<FloatTexture>();
    texture->setImage(image);
    texture->setFiltering(QSGTexture::Nearest);
    texture->setHorizontalWrapMode(QSGTexture::Repeat);
    texture->setVerticalWrapMode(QSGTexture::Repeat);
}

QSGMaterialType* FloatImageViewerMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

int FloatImageViewerMaterial::compare(const QSGMaterial* other) const
{
    Q_ASSERT(other && type() == other->type());
    if (other == this)
    {
        return 0;
    }
    return std::less<const QSGMaterial*>{}(this, other) ? -1 : 1;
}

QSGMaterialShader* FloatImageViewerMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new FloatImageViewerMaterialShader;
}

}  // namespace qtAliceVision
