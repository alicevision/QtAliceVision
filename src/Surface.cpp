#include "Surface.hpp"

#include <aliceVision/camera/IntrinsicBase.hpp>
#include <aliceVision/camera/Pinhole.hpp>

#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/sfmDataIO/sfmDataIO.hpp>

#include <memory>

namespace qtAliceVision
{
	Surface::Surface(int subdivisions)
	{
		setSubdivisions(subdivisions);
	}

	Surface::~Surface()
	{
	}

    bool Surface::loadSfmData(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize,
        bool distortion, bool updateSfmData)
    {
        bool LoadSfm = false;
        if (distortion && (updateSfmData || hasSubsChanged()))
        {
            LoadSfm = true;
            std::string sfmDataFilename;
            aliceVision::sfmData::SfMData sfmData;
            subsChanged(false);

            if (sfmPath().toStdString() != "")
            {
                // Retrieve Sfm Data Path
                sfmDataFilename = sfmPath().toStdString();

                // load SfMData files
                if (!aliceVision::sfmDataIO::Load(sfmData, sfmDataFilename, aliceVision::sfmDataIO::ESfMData::ALL))
                {
                    qWarning() << "The input SfMData file '" << QString::fromUtf8(sfmDataFilename.c_str()) << "' cannot be read.\n";
                    LoadSfm = false;
                }
                if (sfmData.getViews().empty())
                {
                    qWarning() << "The input SfMData file '" << QString::fromUtf8(sfmDataFilename.c_str()) << "' is empty.\n";
                    LoadSfm = false;
                }
                // Make sure there is only one kind of image in dataset
                if (sfmData.getIntrinsics().size() > 1)
                {
                    qWarning() << "Only one intrinsic allowed (" << sfmData.getIntrinsics().size() << " found)";
                    LoadSfm = false;
                }
            }
            else
            {
                LoadSfm = false;
            }

            if (LoadSfm)
            {
                std::shared_ptr<aliceVision::camera::IntrinsicBase> cam = sfmData.getIntrinsics().begin()->second;
                computeGrid(vertices, indices, textureSize, cam);
                updateSfmData = false;

                // Test Landmarks
                aliceVision::sfmData::Landmarks points = sfmData.getLandmarks();
                for (const auto& point : points)
                {
                    qWarning() << point.second.X.x() << point.second.X.y() << point.second.X.z();
                }
            }
        }

        if (!LoadSfm)
        {
            computeGrid(vertices, indices, textureSize, nullptr);
        }

        return LoadSfm;
    }

	void Surface::computeGrid(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize,
        std::shared_ptr<aliceVision::camera::IntrinsicBase> cam)
	{
        if (cam)
        {
            computePrincipalPoint(cam, textureSize);
        }
        computeVerticesGrid(vertices, textureSize, cam);
        computeIndicesGrid(indices);
	}

    void Surface::computeVerticesGrid(QSGGeometry::TexturedPoint2D* vertices, QSize textureSize, 
        std::shared_ptr<aliceVision::camera::IntrinsicBase> cam)
    {
        int compteur = 0;
        for (size_t i = 0; i <= _subdivisions; i++)
        {
            for (size_t j = 0; j <= _subdivisions; j++)
            {
                float x = 0.0f;
                float y = 0.0f;
                if (_vertices.empty())
                {
                    x = i * textureSize.width() / (float)_subdivisions;
                    y = j * textureSize.height() / (float)_subdivisions;
                }
                else
                {
                    x = _vertices[compteur].x();
                    y = _vertices[compteur].y();
                }

                float u = i / (float)_subdivisions;
                float v = j / (float)_subdivisions;

                // Remove Distortion
                if (cam && cam->hasDistortion())
                {
                    const aliceVision::Vec2 undisto_pix(x, y);
                    // TODO : principalPoint
                    const aliceVision::Vec2 disto_pix = cam->get_d_pixel(undisto_pix);
                    vertices[compteur].set(disto_pix.x(), disto_pix.y(), u, v);
                }

                // Equirectangular Convertion

                // Default 
                if (!cam)
                {
                    vertices[compteur].set(x, y, u, v);
                }
                compteur++;
            }
        }
    }

    void Surface::computeIndicesGrid(quint16* indices)
    {
        int pointer = 0;
        for (int j = 0; j < _subdivisions; j++) {
            for (int i = 0; i < _subdivisions; i++) {
                int topLeft = (i * (_subdivisions + 1)) + j;
                int topRight = topLeft + 1;
                int bottomLeft = topLeft + _subdivisions + 1;
                int bottomRight = bottomLeft + 1;
                indices[pointer++] = topLeft;
                indices[pointer++] = bottomLeft;
                indices[pointer++] = topRight;
                indices[pointer++] = topRight;
                indices[pointer++] = bottomLeft;
                indices[pointer++] = bottomRight;
            }
        }
    }

    void Surface::fillVertices(QSGGeometry::TexturedPoint2D* vertices)
    {
        _vertices.clear();
        for (int i = 0; i < _vertexCount; i++)
        {
            QPoint p(vertices[i].x, vertices[i].y);
            _vertices.append(p);
        }

        _verticesChanged = false;
        _reinit = false;
    }

    void Surface::draw(QSGGeometry* geometryLine)
    {
        removeGrid(geometryLine);

        if (_isGridDisplayed)
        {
            int countPoint = 0;
            int index = 0;
            for (size_t i = 0; i <= _subdivisions; i++)
            {
                for (size_t j = 0; j <= _subdivisions; j++)
                {
                    if (i == _subdivisions && j == _subdivisions)
                        continue;

                    // Horizontal Line
                    if (i != _subdivisions)
                    {
                        geometryLine->vertexDataAsPoint2D()[countPoint++].set(_vertices[index].x(), _vertices[index].y());
                        index += _subdivisions + 1;
                        geometryLine->vertexDataAsPoint2D()[countPoint++].set(_vertices[index].x(), _vertices[index].y());
                        index -= _subdivisions + 1;

                        if (j == _subdivisions)
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
    }

	void Surface::setSubdivisions(int sub)
	{
		_subdivisions = sub;
	
		// Update vertexCount and indexCount according to new subdivision count
		_vertexCount = (_subdivisions + 1) * (_subdivisions + 1);
		_indexCount = _subdivisions * _subdivisions * 6;
	}

	int Surface::subdivisions() const
	{
		return _subdivisions;
	}

    void Surface::removeGrid(QSGGeometry* geometryLine)
    {
        for (size_t i = 0; i < geometryLine->vertexCount(); i++)
        {
            geometryLine->vertexDataAsPoint2D()[i].set(0, 0);
        }
    }

    void Surface::computePrincipalPoint(std::shared_ptr<aliceVision::camera::IntrinsicBase> cam, QSize textureSize)
    {
        const aliceVision::Vec2 center(textureSize.width() * 0.5, textureSize.height() * 0.5);
        aliceVision::Vec2 ppCorrection(0.0, 0.0);

        if (aliceVision::camera::isPinhole(cam->getType()))
        {
            ppCorrection = dynamic_cast<aliceVision::camera::Pinhole&>(*cam).getPrincipalPoint();
        }

        _principalPoint.setY(ppCorrection.x());
        _principalPoint.setX(ppCorrection.y());
    }


}  // ns qtAliceVision