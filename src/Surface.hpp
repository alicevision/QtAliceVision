#pragma once
#define _USE_MATH_DEFINES

#include <QQuickItem>
#include <QSGGeometry>
#include <QVariant>
#include <string>

#include <aliceVision/camera/cameraCommon.hpp>
#include <aliceVision/camera/IntrinsicBase.hpp>
#include <aliceVision/numeric/numeric.hpp>

#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/sfmData/CameraPose.hpp>

enum class ViewerType
{
	DEFAULT = 0, HDR, DISTORTION, PANORAMA 
};

namespace qtAliceVision
{
	/**
	 * @brief Discretization of FloatImageViewer surface
	 */
	class Surface
	{
	public:
		Surface(int subdivisions = 4);

		~Surface();

		bool update(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize, bool updateSfmData);
		
		void fillVertices(QSGGeometry::TexturedPoint2D* vertices);

		void drawGrid(QSGGeometry* geometryLine);

		void removeGrid(QSGGeometry* geometryLine);
		
		QPoint principalPoint() const { return _principalPoint; }

		void updateMouseAeraPanoCoords();
		QVariantList mouseAeraPanoCoords() const;


		/*
		* Getters & Setters
		*/

		void setRotationValues(float tx, float ty);

		void setSubdivisions(int sub);
		int subdivisions() const;

		const QList<QPoint>& vertices() const { return _vertices; }
		void clearVertices() { _vertices.clear(); }
		
		const QPoint& vertex(int index) const { return _vertices[index]; }
		QPoint& vertex(int index) { return _vertices[index]; }

		inline int indexCount() const { return _indexCount; }
		inline int vertexCount() const { return _vertexCount; }

		inline QColor gridColor() const { return _gridColor; }
		void setGridColor(const QColor& color) { _gridColor = color; }

		inline bool hasVerticesChanged() const { return _verticesChanged; }
		void verticesChanged(bool change) { _verticesChanged = change; }

		inline bool hasGridChanged() const { return _gridChanged; }
		void gridChanged(bool change) { _gridChanged = change; }

		void gridDisplayed(bool display) { _isGridDisplayed = display; }
		bool isGridDisplayed() const { return _isGridDisplayed; }
		
		void reinitialize(bool reinit) { _reinit = reinit; }
		bool hasReinitialized() const { return _reinit; }

		bool hasSubsChanged() { return _subsChanged; }
		void subsChanged(bool change) { _subsChanged = change; }

		void setSfmPath(const QString& path) { _sfmPath = path; }
		QString sfmPath() const { return _sfmPath; }

		void setIdView(aliceVision::IndexT id) { _idView = id; }
		aliceVision::IndexT idView() const { return _idView; }

		bool isMouseOver() const { return _mouseOver; }
		void setMouseOver(bool state) { _mouseOver = state; }

		// Viewer Type
		ViewerType viewerType() const { return _viewerType; }
		void setViewerType(ViewerType type) { _viewerType = type; }

		bool isPanoViewerEnabled() const;
		bool isDistoViewerEnabled() const;

		/*
		* Static Functions
		*/
		static int downscaleLevelPanorama() { return _downscaleLevelPanorama; }


	private:
		bool loadSfmData();

		void computeGrid(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize, bool updateSfm);

		void computeVerticesGrid(QSGGeometry::TexturedPoint2D* vertices, QSize textureSize,
			aliceVision::camera::IntrinsicBase* intrinsic);
		
		void computeIndicesGrid(quint16* indices);
		
		void computePrincipalPoint(aliceVision::camera::IntrinsicBase* intrinsic, QSize textureSize);

		void rotatePano(aliceVision::Vec3& position);

		void resetRotationValues();

		bool isPanoramaRotated();

	private:
		// Level of downscale for images of a Panorama
		static const int _downscaleLevelPanorama;

		// Vertex Data
		QList<QPoint> _vertices;
		int _subdivisions;
		int _vertexCount;
		int _indexCount;

		// Vertices State
		bool _verticesChanged = true;
		bool _reinit = false;

		// Grid State
		bool _isGridDisplayed = false;
		bool _gridChanged = true;
		QColor _gridColor = QColor(255, 0, 0, 255);
		bool _subsChanged = false;

		// Sfm Data
		aliceVision::sfmData::SfMData _sfmData;
		QString _sfmPath = "";

		// Principal Point Coord
		QPoint _principalPoint = QPoint(0, 0);

		// Id View
		aliceVision::IndexT _idView;

		// Viewer
		ViewerType _viewerType = ViewerType::DEFAULT;

		/* Panorama Variables */
		// Rotation Pano
		aliceVision::Vec2 _rotation = aliceVision::Vec2(0, 0);
		// Mouse Over 
		bool _mouseOver = false;
		// Mouse Area Coordinates
		QVariantList _mouseAreaCoords = { 0, 0, 0, 0 };
	};

}  // ns qtAliceVision