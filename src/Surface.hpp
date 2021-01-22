#pragma once
#define _USE_MATH_DEFINES

#include <QQuickItem>
#include <QSGGeometry>
#include <string>

#include <aliceVision/camera/cameraCommon.hpp>
#include <aliceVision/camera/IntrinsicBase.hpp>
#include <aliceVision/numeric/numeric.hpp>

#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/sfmData/CameraPose.hpp>


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

		bool loadSfmData(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize,
			bool distortion, bool updateSfmData);
		
		void fillVertices(QSGGeometry::TexturedPoint2D* vertices);

		void draw(QSGGeometry* geometryLine);

		void removeGrid(QSGGeometry* geometryLine);
		
		QPoint principalPoint() const { return _principalPoint; }

		/*
		* Getters & Setters
		*/
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

		bool IsPanoViewerEnabled() const { return _panoViewer; }

	private:
		void computeGrid(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize, bool loadSfm);

		void computeVerticesGrid(QSGGeometry::TexturedPoint2D* vertices, QSize textureSize,
			aliceVision::camera::IntrinsicBase* intrinsic);
		
		void computeIndicesGrid(quint16* indices);
		
		void computePrincipalPoint(aliceVision::camera::IntrinsicBase* intrinsic, QSize textureSize);

	private:
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

		bool _panoViewer = true;

	};

}  // ns qtAliceVision