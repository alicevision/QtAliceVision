#pragma once

#include <QQuickItem>
#include <QSGGeometry>
#include <string>

#include <aliceVision/camera/cameraCommon.hpp>
#include <aliceVision/camera/IntrinsicBase.hpp>
#include <aliceVision/numeric/numeric.hpp>


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

	private:
		void computeGrid(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize,
			std::shared_ptr<aliceVision::camera::IntrinsicBase> cam);

		void computeVerticesGrid(QSGGeometry::TexturedPoint2D* vertices, QSize textureSize,
			std::shared_ptr<aliceVision::camera::IntrinsicBase> cam);
		
		void computeIndicesGrid(quint16* indices);
		
		void computePrincipalPoint(std::shared_ptr<aliceVision::camera::IntrinsicBase> cam,
			QSize textureSize);

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

		// Sfm Path
		QString _sfmPath = "";

		// Principal Point Coord
		QPoint _principalPoint = QPoint(0, 0);
	};

}  // ns qtAliceVision