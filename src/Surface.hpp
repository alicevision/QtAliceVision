#pragma once

#include <QQuickItem>
#include <QSGGeometry>
#include <string>

#include <aliceVision/camera/cameraCommon.hpp>
#include <aliceVision/camera/IntrinsicBase.hpp>

namespace qtAliceVision
{
	/**
	 * @brief Discretization of FloatImageViewer surface
	 */
	class Surface
	{
	public:
		Surface(int subdivisions = 5);

		~Surface();

		void ComputeGrid(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize, 
			aliceVision::camera::IntrinsicBase* cam);

		void FillVertices(QSGGeometry::TexturedPoint2D* vertices);

		void Draw(QSGGeometry* geometryLine);

		void SetSubdivisions(int sub);
		int Subdivisions() const;

		const QList<QPoint>& Vertices() const { return _vertices; }
		
		const QPoint& Vertex(int index) const { return _vertices[index]; }
		QPoint& Vertex(int index) { return _vertices[index]; }
		void ClearVertices() { _vertices.clear(); }

		inline QColor GridColor() const { return _gridColor; }
		void SetGridColor(const QColor& color) { _gridColor = color; }
		inline int IndexCount() const { return _indexCount; }
		inline int VertexCount() const { return _vertexCount; }

		inline bool HasVerticesChanged() const { return _verticesChanged; }
		void VerticesChanged(bool change) { _verticesChanged = change; }

		inline bool HasGridChanged() const { return _gridChanged; }
		void GridChanged(bool change) { _gridChanged = change; }

		void GridDisplayed(bool display) { _isGridDisplayed = display; }
		bool IsGridDisplayed() const { return _isGridDisplayed; }
		
		void Reinitialize(bool reinit) { _reinit = reinit; }
		bool HasReinitialized() const { return _reinit; }

		void RemoveGrid(QSGGeometry* geometryLine);

		bool HasSubsChanged() {
			if (_subsChanged)
			{
				_subsChanged = false;
				return true;
			}
			else
			{
				_subsChanged = true;
				return false;
			}
		}

		void SubsChanged(bool change) { _subsChanged = change; }

	private:
		void ComputeVerticesGrid(QSGGeometry::TexturedPoint2D* vertices, QSize textureSize,
			aliceVision::camera::IntrinsicBase* cam);
		void ComputeIndicesGrid(quint16* indices);
		void RemoveDistortion(QSGGeometry::TexturedPoint2D* vertices, QSize textureSize);

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

	};

}  // ns qtAliceVision