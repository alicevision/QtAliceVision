#include "Surface.hpp"

#include <aliceVision/camera/IntrinsicBase.hpp>
#include <aliceVision/camera/Pinhole.hpp>

#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/sfmDataIO/sfmDataIO.hpp>

// Import M_PI
#include <math.h>
#include <memory>

namespace qtAliceVision
{
    aliceVision::Vec2 toEquirectangular(const aliceVision::Vec3& spherical, int width, int height);

    const int Surface::_downscaleLevelPanorama = 2;
    const int Surface::_panoramaWidth = 3000;
    const int Surface::_panoramaHeight = 1000;

	Surface::Surface(int subdivisions)
	{
		setSubdivisions(subdivisions);
	}

	Surface::~Surface()
	{
	}

    bool Surface::update(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize, bool updateSfmData)
    {
        // Load Sfm Data File only if needed
        if ((isDistoViewerEnabled() && (updateSfmData || hasSubsChanged()) ) // Disto Viewer conditions
         || (isPanoViewerEnabled() && updateSfmData) )                       // Pano Viewer conditions
        {
            updateSfmData = loadSfmData();
        }

        // Compute Vertices coordinates and Indices order
        computeGrid(vertices, indices, textureSize, updateSfmData);

        // If Panorama has been rotated, reset values and return true
        if (isPanoramaRotated())
        {
            resetRotationValues();
            return true;
        }

        return updateSfmData;
    }

    bool Surface::isPanoViewerEnabled() const
    {
        return _viewerType == ViewerType::PANORAMA;
    }

    bool Surface::isDistoViewerEnabled() const
    {
        return _viewerType == ViewerType::DISTORTION;
    }

	void Surface::computeGrid(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize, bool updateSfmData)
	{
        qWarning() << "Compute Grid ============================";

        // Retrieve intrisics only if sfmData has been updated, and then Compute vertices
        aliceVision::camera::IntrinsicBase* intrinsic = nullptr;
        if (updateSfmData || isPanoramaRotated())
        {
            std::set<aliceVision::IndexT> intrinsicsIndices = _sfmData.getReconstructedIntrinsics();
            intrinsic = _sfmData.getIntrinsicPtr(*intrinsicsIndices.begin());

            if (intrinsic)
            {
                qWarning() << "With Intrinsics";
                computePrincipalPoint(intrinsic, textureSize);
                computeVerticesGrid(vertices, textureSize, intrinsic);
            }
        }
        // If there is no sfm data update or intrinsics are invalid, keep the same vertices
        if ((!updateSfmData && !isPanoramaRotated()) || !intrinsic)
        {
            qWarning() << "Without Intrinsics";
            computeVerticesGrid(vertices, textureSize, nullptr);
        }
        
        // TODO : compute indices only if subs has changed
        computeIndicesGrid(indices);
	}

    void Surface::computeVerticesGrid(QSGGeometry::TexturedPoint2D* vertices, QSize textureSize, 
        aliceVision::camera::IntrinsicBase* intrinsic)
    {
        // Retrieve pose if Panorama Viewer is enable
        aliceVision::sfmData::CameraPose pose;
        if (isPanoViewerEnabled() && intrinsic)
        {
            // Downscale image according to downscale level
            textureSize *= pow(2.0, _downscaleLevelPanorama);
            aliceVision::sfmData::View view = _sfmData.getView(_idView);
            pose = _sfmData.getPose(view);
        }

        bool fillCoordsSphere = _coordsSphere.empty();
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

                // TODO : update when subs change
                float u = i / (float)_subdivisions;
                float v = j / (float)_subdivisions;

                // Remove Distortion only if sfmData has been updated
                if (intrinsic && intrinsic->hasDistortion())
                {
                    const aliceVision::Vec2 undisto_pix(x, y);
                    const aliceVision::Vec2 disto_pix = intrinsic->get_d_pixel(undisto_pix);
                    vertices[compteur].set(disto_pix.x(), disto_pix.y(), u, v);
                }

                // Equirectangular Convertion only if sfmData has been updated
                if (isPanoViewerEnabled() && intrinsic)
                {
                    // Compute pixel coordinates on the Unit Sphere
                    if (fillCoordsSphere) {
                        // Image System Coordinates
                        aliceVision::Vec2 uvCoord(x, y);
                        const auto& transfromPose = pose.getTransform();
                        _coordsSphere.push_back(aliceVision::camera::applyIntrinsicExtrinsic(transfromPose, intrinsic, uvCoord));
                    }

                    // Rotate Panorama if some rotation values exist
                    if (isPanoramaRotated())
                    {
                        rotatePano(compteur);
                    }

                    // Compute pixel coordinates in the panorama coordinate system
                    aliceVision::Vec2 coordPano = toEquirectangular(_coordsSphere[compteur], _panoramaWidth, _panoramaHeight);
                    
                    // If image is on the seem
                    if (compteur > 1 && abs(coordPano.x() - vertices[compteur - 1].x) > _panoramaWidth * 0.9)
                    {
                        coordPano.x() -= _panoramaWidth;
                    }
                    vertices[compteur].set(coordPano.x(), coordPano.y(), u, v);


                }

                // Default 
                if (!intrinsic)
                {
                    vertices[compteur].set(x, y, u, v);
                }
                compteur++;
            }
        }
        // End for loop
        
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
            //qWarning() << p;
            _vertices.append(p);
        }
        
        updateMouseAeraPanoCoords();

        _verticesChanged = false;
        _reinit = false;
    }

    void Surface::drawGrid(QSGGeometry* geometryLine)
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

    bool Surface::loadSfmData()
    {
        qWarning() << "==============================";
        qWarning() << "LOADING SFM DATA";
        qWarning() << "==============================";
        bool LoadSfm = true;
        subsChanged(false);

        if (_sfmPath.toStdString() != "")
        {
            // Clear sfmData
            _sfmData.clear();

            // load SfMData files
            if (!aliceVision::sfmDataIO::Load(_sfmData, _sfmPath.toStdString(), aliceVision::sfmDataIO::ESfMData::ALL))
            {
                qWarning() << "The input SfMData file '" << _sfmPath << "' cannot be read.\n";
                return false;
            }
            if (_sfmData.getViews().empty())
            {
                qWarning() << "The input SfMData file '" << _sfmPath << "' is empty.\n";
                return false;
            }
            // Make sure there is only one kind of image in dataset
            if (_sfmData.getIntrinsics().size() > 1)
            {
                qWarning() << "Only one intrinsic allowed (" << _sfmData.getIntrinsics().size() << " found)";
                return false;
            }
        }
        else
        {
            return false;
        }

        return true;
    }

    void Surface::computePrincipalPoint(aliceVision::camera::IntrinsicBase* intrinsic, QSize textureSize)
    {
        const aliceVision::Vec2 center(textureSize.width() * 0.5, textureSize.height() * 0.5);
        aliceVision::Vec2 ppCorrection(0.0, 0.0);

        if (aliceVision::camera::isPinhole(intrinsic->getType()))
        {
            ppCorrection = dynamic_cast<aliceVision::camera::Pinhole&>(*intrinsic).getPrincipalPoint();
        }

        _principalPoint.setY(ppCorrection.x());
        _principalPoint.setX(ppCorrection.y());
    }

    void Surface::setRotationValues(float tx, float ty)
    {
        _rotation.x() = tx;
        _rotation.y() = ty;
    }

    void Surface::resetRotationValues()
    {
        _rotation = aliceVision::Vec2(0, 0);
    }

    void Surface::rotatePano(int index)
    {
        // Translation Left - Right
        double yaw = _rotation.x();
        qWarning() << "ROTATE PANO" << yaw;

        Eigen::AngleAxisd yawAngle(yaw, Eigen::Vector3d::UnitY());
        // Translation Top - Bottom
        //double pitch = _rotation.y();
        //Eigen::AngleAxisd pitchAngle(pitch, Eigen::Vector3d::UnitX());

        _coordsSphere[index] = yawAngle * _coordsSphere[index];
    }

    bool Surface::isPanoramaRotated()
    {
        if (isPanoViewerEnabled())
            return _rotation != aliceVision::Vec2(0, 0);
        else
            return false;
    }

    void Surface::updateMouseAeraPanoCoords()
    {
        _mouseAreaCoords[0] = _vertices[_vertexCount - 1].x();                    // x
        _mouseAreaCoords[1] = _vertices[0].y();                                   // y
        _mouseAreaCoords[2] = _vertices[0].x() - _vertices[_vertexCount - 1].x(); // width
        _mouseAreaCoords[3] = _vertices[_vertexCount - 1].y() - _vertices[0].y(); // height
    }

    QVariantList Surface::mouseAeraPanoCoords() const
    {
        return _mouseAreaCoords;
    }

    /*
    * Utils Functions
    */

    aliceVision::Vec2 toEquirectangular(const aliceVision::Vec3& spherical, int width, int height) {

        double vertical_angle = asin(spherical(1));
        double horizontal_angle = atan2(spherical(0), spherical(2));

        double latitude = ((vertical_angle + M_PI_2) / M_PI) * height;
        double longitude = ((horizontal_angle + M_PI) / (2.0 * M_PI)) * width;

        return aliceVision::Vec2(longitude, latitude);
    }


}  // ns qtAliceVision