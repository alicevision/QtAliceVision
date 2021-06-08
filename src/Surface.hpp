#pragma once
#define _USE_MATH_DEFINES

#include <QQuickItem>
#include <QSGGeometry>
#include <QVariant>
#include <string>
#include <vector>

#include <aliceVision/camera/cameraCommon.hpp>
#include <aliceVision/camera/IntrinsicBase.hpp>
#include <aliceVision/numeric/numeric.hpp>

#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/sfmData/CameraPose.hpp>



namespace qtAliceVision
{

enum class ViewerType
{
	DEFAULT = 0, HDR, DISTORTION, PANORAMA
};

/**
 * @brief Discretization of FloatImageViewer surface
 */
class Surface : public QObject
{
Q_OBJECT
	Q_PROPERTY(QColor gridColor READ getGridColor WRITE setGridColor NOTIFY gridColorChanged);

public:
	QColor getGridColor()
	{
		return _gridColor;
	}
	void setGridColor(const QColor& color)
	{
		_gridColor = color;
		Q_EMIT gridColorChanged(color);
	}
	Q_SIGNAL void gridColorChanged(QColor);


public:
	Surface(int subdivisions = 4, QObject* parent = nullptr);
	Surface& operator=(const Surface& other) = default;
	~Surface();

	bool update(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize, bool updateSfmData, int downscaleLevel = 0);
		
	void fillVertices(QSGGeometry::TexturedPoint2D* vertices);

	void drawGrid(QSGGeometry* geometryLine);

	void removeGrid(QSGGeometry* geometryLine);
		
	QPoint principalPoint() const { return _principalPoint; }


	/*
	* Getters & Setters
	*/
	double pitch() const { return _pitch; }
	double yaw() const { return _yaw; }

	void setRotationValues(float yaw, float pitch);
	void incrementRotationValues(float yaw, float pitch);

	void setSubdivisions(int sub);
	int subdivisions() const;

	const QList<QPoint>& vertices() const { return _vertices; }
	void clearVertices() { _vertices.clear(); _coordsSphereDefault.clear(); }
		
	const QPoint& vertex(int index) const { return _vertices[index]; }
	QPoint& vertex(int index) { return _vertices[index]; }
	const quint32 index(int index) { return _indices[index]; }

	inline int indexCount() const { return _indexCount; }
	inline int vertexCount() const { return _vertexCount; }

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

	//static int downscaleLevelPanorama() { return _downscaleLevelPanorama; }					// --> FIV Not Static
	//static void setDownscaleLevelPanorama(int level) { _downscaleLevelPanorama = level; }	// --> FIV Not Static


private:
	bool loadSfmData();

	void computeGrid(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize, bool updateSfm, int downscaleLevel = 0);

	void computeVerticesGrid(QSGGeometry::TexturedPoint2D* vertices, QSize textureSize,
		aliceVision::camera::IntrinsicBase* intrinsic, int downscaleLevel = 0);
		
	void computeIndicesGrid(quint16* indices);
		
	void computePrincipalPoint(aliceVision::camera::IntrinsicBase* intrinsic, QSize textureSize);

	void rotatePano(aliceVision::Vec3& coordSphere);

private:
	/*
	* Static Variables
	*/
	// Level of downscale for images of a Panorama
	//static int _downscaleLevelPanorama;	// --> Not Static
	static const int _panoramaWidth;	// --> Not Static
	static const int _panoramaHeight;	// --> Not Static

	// Vertex Data
	QList<QPoint> _vertices;
	QList<quint16> _indices;
	int _subdivisions;
	int _vertexCount;
	int _indexCount;
	std::vector<std::pair<int, int> > _deletedColIndex;

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

	/* Euler angle in radians */
	double _pitch = 0.0;
	double _yaw = 0.0;
	// Coordinates on Unit Sphere without any rotation
	std::vector<aliceVision::Vec3> _coordsSphereDefault;
	// Mouse Over 
	bool _mouseOver = false;
	// If panorama is currently rotating
	bool _isPanoramaRotating = false;
	// Mouse Area Coordinates
	QVariantList _mouseAreaCoords = { 0, 0, 0, 0 };
};

}  // ns qtAliceVision