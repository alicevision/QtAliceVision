#pragma once
#define _USE_MATH_DEFINES

#include <QQuickItem>
#include <QSGGeometry>
#include <QVariant>
#include <string>
#include <vector>
#include <MSfMData.hpp>

#include <aliceVision/camera/IntrinsicBase.hpp>
#include <aliceVision/sfmData/SfMData.hpp>

namespace qtAliceVision
{

/**
 * @brief Discretization of FloatImageViewer surface
 */
class Surface : public QObject
{
Q_OBJECT

	//Q_PROPERTIES

	Q_PROPERTY(bool displayGrid READ getDisplayGrid WRITE setDisplayGrid NOTIFY displayGridChanged);
	
	Q_PROPERTY(QColor gridColor READ getGridColor WRITE setGridColor NOTIFY gridColorChanged);

	Q_PROPERTY(int gridOpacity READ getGridOpacity WRITE setGridOpacity NOTIFY gridOpacityChanged);

	Q_PROPERTY(bool mouseOver READ getMouseOver WRITE setMouseOver NOTIFY mouseOverChanged);

	Q_PROPERTY(qtAliceVision::MSfMData* msfmData READ getMSfmData WRITE setMSfmData NOTIFY sfmDataChanged)

	Q_PROPERTY(EViewerType viewerType WRITE setViewerType NOTIFY viewerTypeChanged);

	Q_PROPERTY(QList<QPoint> vertices READ vertices NOTIFY verticesChanged);

	Q_PROPERTY(int subdivisions READ getSubdivisions WRITE setSubdivisions NOTIFY subdivisionsChanged);

public:
	Surface(int subdivisions = 12, QObject* parent = nullptr);
	Surface& operator=(const Surface& other) = default;
	~Surface();

	void update(QSGGeometry::TexturedPoint2D * vertices, quint16 * indices, QSize textureSize, int downscaleLevel = 0);

	// Q_INVOKABLES
	Q_INVOKABLE QPoint getPrincipalPoint() { return _principalPoint; };
	Q_INVOKABLE bool isMouseInside(float mx, float my);
	Q_INVOKABLE void setIdView(int id);
	Q_INVOKABLE double getPitch();
	Q_INVOKABLE double getYaw();
	Q_INVOKABLE double getRoll();

	// Pass angles in radians
	Q_INVOKABLE void incrementEulerAngles(float yawRadians, float pitchRadians);
	// Pass angles in degrees
	Q_INVOKABLE void setEulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees);

	// GRID
	void computeGrid(QSGGeometry* geometryLine);
	void removeGrid(QSGGeometry* geometryLine);

	// GRID COLOR
	QColor getGridColor() const { return _gridColor; }
	void setGridColor(const QColor& color);
	Q_SIGNAL void gridColorChanged(QColor);

	// GRID OPACITY
	int getGridOpacity() const { return _gridOpacity; }
	void setGridOpacity(const int& opacity);
	Q_SIGNAL void gridOpacityChanged(int);

	// DISPLAY GRID
	bool getDisplayGrid() const { return _displayGrid && isDistortionViewerEnabled(); }
	void setDisplayGrid(bool display);
	Q_SIGNAL void displayGridChanged();

	// MOUSE OVER
	bool getMouseOver() const { return _mouseOver; }
	void setMouseOver(bool state);
	Q_SIGNAL void mouseOverChanged();

	// SUBDIVISION
	int getSubdivisions() const { return _subdivisions; }
	void setSubdivisions(int newSubdivisions);
	Q_SIGNAL void subdivisionsChanged();

	// MSfmData
	MSfMData* getMSfmData() { return _msfmData; }
	void setMSfmData(MSfMData* sfmData);
	Q_SIGNAL void sfmDataChanged();

	// VIEWER TYPE
	enum class EViewerType : quint8 { DEFAULT = 0, HDR, DISTORTION, PANORAMA };
	Q_ENUM(EViewerType)

	void setViewerType(EViewerType type);
	bool isPanoramaViewerEnabled() const;
	bool isDistortionViewerEnabled() const;
	bool isHDRViewerEnabled() const;

	Q_SIGNAL void viewerTypeChanged();

	// VERTICES
	const QList<QPoint>& vertices() const { return _vertices; }
	inline bool hasVerticesChanged() const { return _verticesChanged; }
	void setVerticesChanged(bool change) { _verticesChanged = change; }
	void clearVertices() { _vertices.clear(); _defaultSphereCoordinates.clear(); }

	inline int indexCount() const { return _indexCount; }
	inline int vertexCount() const { return _vertexCount; }

	Q_SIGNAL void verticesChanged();

	// SUBDIVISIONS
	bool hasSubdivisionsChanged() { return _subdivisionsChanged; }
	void setHasSubdivisionsChanged(bool state) { _subdivisionsChanged = state; }

	void setNeedToUseIntrinsic(bool state) { _needToUseIntrinsic = state; }

	void fillVertices(QSGGeometry::TexturedPoint2D* vertices);

private:

	void computeGrid(QSGGeometry::TexturedPoint2D* vertices, quint16* indices, QSize textureSize, int downscaleLevel = 0);

	void computeVerticesGrid(QSGGeometry::TexturedPoint2D* vertices, QSize textureSize,
		aliceVision::camera::IntrinsicBase* intrinsic, int downscaleLevel = 0);
		
	void computeIndicesGrid(quint16* indices);
	
	void computePrincipalPoint(aliceVision::camera::IntrinsicBase* intrinsic);

	void rotatePanorama(aliceVision::Vec3& coordSphere);

	void updateSubdivisions(int sub);

	bool isPointValid(int i, int j) const;

	void resetValuesVertexEnabled();

	// Get pitch / yaw / roll in radians and return degrees angle in the correct interval
	double getEulerAngleDegrees(double angleRadians);


private:
	const int _panoramaWidth = 3000;
	const int _panoramaHeight = 1500;

	// Vertex Data
	QList<QPoint> _vertices;
	QList<quint16> _indices;
	int _subdivisions;
	int _vertexCount;
	int _indexCount;
	std::vector<std::vector<bool> > _vertexEnabled;

	// Vertices State
	bool _verticesChanged = true;

	// Grid State
	bool _displayGrid;
	QColor _gridColor = QColor(255, 0, 0, 255);
	int _gridOpacity = 255;
	bool _subdivisionsChanged = false;

	// SfmData
	MSfMData* _msfmData = nullptr;
	// sfmData is ready to use
	bool _sfmLoaded = false;
	// Compute vertices grid with intrinsic
	bool _needToUseIntrinsic = true;

	// Principal Point Coord
	QPoint _principalPoint = QPoint(0, 0);

	// Id View
	aliceVision::IndexT _idView;

	// Viewer
	EViewerType _viewerType = EViewerType::DEFAULT;

	// Euler angle in radians
	double _pitch = 0.0;
	double _yaw = 0.0;
	double _roll = 0.0;
	// Coordinates on Unit Sphere without any rotation
	std::vector<aliceVision::Vec3> _defaultSphereCoordinates;
	// Mouse Over 
	bool _mouseOver = false;
	// If panorama is currently rotating
	bool _isPanoramaRotating = false;
};

}