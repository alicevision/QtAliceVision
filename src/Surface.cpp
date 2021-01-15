#include "Surface.hpp"

#include <aliceVision/numeric/numeric.hpp>


namespace qtAliceVision
{
	Surface::Surface(int subdivisions)
	{
		SetSubdivisions(subdivisions);
	}

	Surface::~Surface()
	{
	}

	void Surface::ComputeGrid(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize,
        aliceVision::camera::IntrinsicBase* cam)
	{
        ComputeVerticesGrid(vertices, textureSize, cam);
        // Change only if subs are changed
        ComputeIndicesGrid(indices);
	}

    void Surface::ComputeVerticesGrid(QSGGeometry::TexturedPoint2D* vertices, QSize textureSize, 
        aliceVision::camera::IntrinsicBase* cam)
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
                    qWarning() << x << y;
                    const aliceVision::Vec2 undisto_pix(x, y);
                    const aliceVision::Vec2 disto_pix = cam->get_d_pixel(undisto_pix);
                    vertices[compteur].set(disto_pix.x(), disto_pix.y(), u, v);
                }
                else
                {
                    vertices[compteur].set(x, y, u, v);
                }
                compteur++;
            }
        }

    }

    void Surface::ComputeIndicesGrid(quint16* indices)
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

    void Surface::FillVertices(QSGGeometry::TexturedPoint2D* vertices)
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

    void Surface::Draw(QSGGeometry* geometryLine)
    {
        RemoveGrid(geometryLine);

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

	void Surface::SetSubdivisions(int sub)
	{
		_subdivisions = sub;
	
		// Update vertexCount and indexCount according to new subdivision count
		_vertexCount = (_subdivisions + 1) * (_subdivisions + 1);
		_indexCount = _subdivisions * _subdivisions * 6;
	}

	int Surface::Subdivisions() const
	{
		return _subdivisions;
	}

    void Surface::RemoveGrid(QSGGeometry* geometryLine)
    {
        for (size_t i = 0; i < geometryLine->vertexCount(); i++)
        {
            geometryLine->vertexDataAsPoint2D()[i].set(0, 0);
        }
    }


}  // ns qtAliceVision