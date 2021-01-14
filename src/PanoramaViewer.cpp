#include "PanoramaViewer.hpp"
#include "FloatImageViewer.hpp"
#include "FloatTexture.hpp"
#include "ShaderImageViewer.hpp"

#include <QSGGeometryNode>
#include <QSGGeometry>
#include <QSGSimpleMaterial>
#include <QSGSimpleMaterialShader>
#include <QSGTexture>
#include <QThreadPool>
#include <QSGFlatColorMaterial>

#include <aliceVision/image/Image.hpp>
#include <aliceVision/image/resampling.hpp>
#include <aliceVision/image/io.hpp>
#include <aliceVision/camera/cameraUndistortImage.hpp>


namespace qtAliceVision
{
    PanoramaViewer::PanoramaViewer(QQuickItem* parent)
        : QQuickItem(parent)
    {
        setFlag(QQuickItem::ItemHasContents, true);
        connect(this, &PanoramaViewer::gammaChanged, this, &PanoramaViewer::update);
        connect(this, &PanoramaViewer::gainChanged, this, &PanoramaViewer::update);
        connect(this, &PanoramaViewer::textureSizeChanged, this, &PanoramaViewer::update);
        connect(this, &PanoramaViewer::sourceSizeChanged, this, &PanoramaViewer::update);
        connect(this, &PanoramaViewer::channelModeChanged, this, &PanoramaViewer::update);
        connect(this, &PanoramaViewer::imageChanged, this, &PanoramaViewer::update);
        connect(this, &PanoramaViewer::sourceChanged, this, &PanoramaViewer::reload);
        connect(this, &PanoramaViewer::verticesChanged, this, &PanoramaViewer::update);
        connect(this, &PanoramaViewer::gridColorChanged, this, &PanoramaViewer::update);
    }

    PanoramaViewer::~PanoramaViewer()
    {
    }

    void PanoramaViewer::setLoading(bool loading)
    {
        if (_loading == loading)
        {
            return;
        }
        _loading = loading;
        Q_EMIT loadingChanged();
    }


    QVector4D PanoramaViewer::pixelValueAt(int x, int y)
    {
        if (!_image)
        {
            // qWarning() << "[QtAliceVision] PanoramaViewer::pixelValueAt(" << x << ", " << y << ") => no valid image";
            return QVector4D(0.0, 0.0, 0.0, 0.0);
        }
        else if (x < 0 || x >= _image->Width() ||
            y < 0 || y >= _image->Height())
        {
            // qWarning() << "[QtAliceVision] PanoramaViewer::pixelValueAt(" << x << ", " << y << ") => out of range";
            return QVector4D(0.0, 0.0, 0.0, 0.0);
        }
        aliceVision::image::RGBAfColor color = (*_image)(y, x);
        // qWarning() << "[QtAliceVision] PanoramaViewer::pixelValueAt(" << x << ", " << y << ") => valid pixel: " << color(0) << ", " << color(1) << ", " << color(2) << ", " << color(3);
        return QVector4D(color(0), color(1), color(2), color(3));
    }

    void PanoramaViewer::reload()
    {
        if (_clearBeforeLoad)
        {
            _image.reset();
            _imageChanged = true;
            Q_EMIT imageChanged();
        }
        _outdated = false;

        if (!_source.isValid())
        {
            _image.reset();
            _imageChanged = true;
            Q_EMIT imageChanged();
            return;
        }

        if (!_loading)
        {
            setLoading(true);

            // async load from file
            auto ioRunnable = new FloatImageIORunnable(_source);
            connect(ioRunnable, &FloatImageIORunnable::resultReady, this, &PanoramaViewer::onResultReady);
            QThreadPool::globalInstance()->start(ioRunnable);
        }
        else
        {
            _outdated = true;
        }
    }

    void PanoramaViewer::onResultReady(QSharedPointer<FloatImage> image, QSize sourceSize, const QVariantMap& metadata)
    {
        setLoading(false);

        if (_outdated)
        {
            // another request has been made while io thread was working
            _image.reset();
            reload();
            return;
        }

        _image = image;
        _imageChanged = true;
        Q_EMIT imageChanged();

        _sourceSize = sourceSize;
        Q_EMIT sourceSizeChanged();

        _metadata = metadata;
        Q_EMIT metadataChanged();
    }

    void PanoramaViewer::updatePaintGrid(QSGNode* oldNode, QSGNode* node)
    {

    }

    QSGNode* PanoramaViewer::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* data)
    {
        QVector4D channelOrder(0.f, 1.f, 2.f, 3.f);

        QSGGeometryNode* root = static_cast<QSGGeometryNode*>(oldNode);
        QSGSimpleMaterial<ShaderData>* material = nullptr;

        int subdivision = 10;
        int vertexCount = (subdivision + 1) * (subdivision + 1);
        int indexCount = subdivision * subdivision * 6;
        QSGGeometry* geometryLine = nullptr;

        if (!root)
        {
            root = new QSGGeometryNode;
            auto geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), vertexCount, indexCount);

            geometry->setDrawingMode(GL_TRIANGLES);

            geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
            geometry->setVertexDataPattern(QSGGeometry::StaticPattern);

            root->setGeometry(geometry);
            root->setFlags(QSGNode::OwnsGeometry);

            material = ImageViewerShader::createMaterial();
            root->setMaterial(material);
            root->setFlags(QSGNode::OwnsMaterial);
            {
                /* Geometry and Material for the Grid */
                auto node = new QSGGeometryNode;
                auto material = new QSGFlatColorMaterial;
                material->setColor(_gridColor);
                {
                    geometryLine = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), indexCount);
                    geometryLine->setDrawingMode(GL_LINES);
                    geometryLine->setLineWidth(3);

                    node->setGeometry(geometryLine);
                    node->setFlags(QSGNode::OwnsGeometry);
                    node->setMaterial(material);
                    node->setFlags(QSGNode::OwnsMaterial);
                }
                root->appendChildNode(node);
            }
            updatePaintGrid(oldNode, root);
        }
        else
        {
            material = static_cast<QSGSimpleMaterial<ShaderData>*>(root->material());

            QSGGeometryNode* rootGrid = static_cast<QSGGeometryNode*>(oldNode->childAtIndex(0));
            auto mat = static_cast<QSGFlatColorMaterial*>(rootGrid->activeMaterial());
            mat->setColor(_gridColor);
            geometryLine = rootGrid->geometry();
        }

        // enable Blending flag for transparency for RGBA
        material->setFlag(QSGMaterial::Blending, _channelMode == EChannelMode::RGBA);

        // change channelOrder according to channelMode
        switch (_channelMode)
        {
        case EChannelMode::R:
            channelOrder = QVector4D(0.f, 0.f, 0.f, -1.f);
            break;
        case EChannelMode::G:
            channelOrder = QVector4D(1.f, 1.f, 1.f, -1.f);
            break;
        case EChannelMode::B:
            channelOrder = QVector4D(2.f, 2.f, 2.f, -1.f);
            break;
        case EChannelMode::A:
            channelOrder = QVector4D(3.f, 3.f, 3.f, -1.f);
            break;
        }

        bool updateGeometry = false;
        material->state()->gamma = _gamma;
        material->state()->gain = _gain;
        material->state()->channelOrder = channelOrder;

        if (_imageChanged)
        {
            QSize newTextureSize;
            auto texture = std::make_unique<FloatTexture>();
            if (_image)
            {
                texture->setImage(_image);
                texture->setFiltering(QSGTexture::Nearest);
                texture->setHorizontalWrapMode(QSGTexture::Repeat);
                texture->setVerticalWrapMode(QSGTexture::Repeat);
                newTextureSize = texture->textureSize();
            }
            material->state()->texture = std::move(texture);

            _imageChanged = false;

            if (_textureSize != newTextureSize)
            {
                _textureSize = newTextureSize;
                updateGeometry = true;
                Q_EMIT textureSizeChanged();
            }
        }

        const auto newBoundingRect = boundingRect();
        if (updateGeometry || _boundingRect != newBoundingRect)
        {
            _boundingRect = newBoundingRect;

            const float windowRatio = _boundingRect.width() / _boundingRect.height();
            const float textureRatio = _textureSize.width() / float(_textureSize.height());
            QRectF geometryRect = _boundingRect;
            if (windowRatio > textureRatio)
            {
                geometryRect.setWidth(geometryRect.height() * textureRatio);
            }
            else
            {
                geometryRect.setHeight(geometryRect.width() / textureRatio);
            }
            geometryRect.moveCenter(_boundingRect.center());

            static const int MARGIN = 0;
            geometryRect = geometryRect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);

            QSGGeometry::updateTexturedRectGeometry(root->geometry(), geometryRect, QRectF(0, 0, 1, 1));
            root->markDirty(QSGNode::DirtyGeometry);
        }

        /* Process all the data to calculate the grid */
        if (_verticesChanged)
        {
            srand(time(0));

            // Retrieve Vertices and Index Data
            QSGGeometry::TexturedPoint2D* vertices = root->geometry()->vertexDataAsTexturedPoint2D();
            quint16* indices = root->geometry()->indexDataAsUShort();

            // Coordinates of the Grid
            int compteur = 0;
            for (size_t i = 0; i <= subdivision; i++)
            {
                for (size_t j = 0; j <= subdivision; j++)
                {
                    float x = 0.0f;
                    float y = 0.0f;
                    if (_vertices.empty())
                    {
                        x = i * _textureSize.width() / (float)subdivision;
                        y = j * _textureSize.height() / (float)subdivision;
                    }
                    else
                    {
                        x = _vertices[compteur].x();
                        y = _vertices[compteur].y();
                    }

                    // Add random deformation on each point
                    if (_randomCP)
                    {
                        x += rand() % 40 + (-20);   // Random number between -20 and 20
                        y += rand() % 40 + (-20);
                    }

                    float u = i / (float)subdivision;
                    float v = j / (float)subdivision;

                    //qWarning() << "(" << x << "," << y << ")\n";

                    vertices[compteur].set(x, y, u, v);
                    compteur++;
                }
            }

            // Indices
            int pointer = 0;
            for (int j = 0; j < subdivision; j++) {
                for (int i = 0; i < subdivision; i++) {
                    int topLeft = (i * (subdivision + 1)) + j;
                    int topRight = topLeft + 1;
                    int bottomLeft = topLeft + subdivision + 1;
                    int bottomRight = bottomLeft + 1;
                    indices[pointer++] = topLeft;
                    indices[pointer++] = bottomLeft;
                    indices[pointer++] = topRight;
                    indices[pointer++] = topRight;
                    indices[pointer++] = bottomLeft;
                    indices[pointer++] = bottomRight;
                }
            }

            root->geometry()->markIndexDataDirty();
            root->geometry()->markVertexDataDirty();
            root->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);

            // Fill the _vertices array
            _vertices.clear();
            for (int i = 0; i < vertexCount; i++)
            {
                QPoint p(vertices[i].x, vertices[i].y);
                _vertices.append(p);
            }

            if (_reinit || _randomCP)
                Q_EMIT verticesChanged(true);
            else
                Q_EMIT verticesChanged(false);


            _verticesChanged = false;
            _randomCP = false;
            _reinit = false;

        }

        /* Draw the grid */
        if (_gridChanged)
        {
            for (size_t i = 0; i < geometryLine->vertexCount(); i++)
            {
                geometryLine->vertexDataAsPoint2D()[i].set(0, 0);
            }
            
            if (_isGridDisplayed)
            {
                int countPoint = 0;
                int index = 0;
                for (size_t i = 0; i <= subdivision; i++)
                {
                    for (size_t j = 0; j <= subdivision; j++)
                    {
                        if (i == subdivision && j == subdivision)
                            continue;

                        // Horizontal Line
                        if (i != subdivision)
                        {
                            geometryLine->vertexDataAsPoint2D()[countPoint++].set(_vertices[index].x(), _vertices[index].y());
                            index += subdivision + 1;
                            geometryLine->vertexDataAsPoint2D()[countPoint++].set(_vertices[index].x(), _vertices[index].y());
                            index -= subdivision + 1;

                            if (j == subdivision)
                            {
                                index++;
                                continue;
                            }
                        }

                        // Vertical Line
                        geometryLine->vertexDataAsPoint2D()[countPoint++].set(_vertices[index].x(), _vertices[index].y());
                        index++;
                        geometryLine->vertexDataAsPoint2D()[countPoint++].set(_vertices[index].x(), _vertices[index].y());
                    }
                }
            }
            _gridChanged = false;
            Q_EMIT verticesChanged(false);
        }


        root->childAtIndex(0)->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);

        return root;
    }

}  // qtAliceVision
