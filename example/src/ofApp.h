#pragma once

#include "ofMain.h"
#include "ofPolygonTriangulation.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp {

public:
	void setup();
	void exit();
	void update();
	void draw();

private:
	bool m_IsMonotone;
	ofDoublyConnectedEdgeList m_Dcel;
	ofPolygonTriangulation m_PolygonTriangulation;
	ofPolyline m_Line;
	ofMesh m_Mesh;
	vector<glm::vec3> m_Vertices;
	vector<ofIndexType> m_Indices;

	// GUI
	size_t m_NumPoints { 9 };
	ofxPanel m_Gui;
	ofxToggle m_IsMonotoneToggle;
	ofxIntSlider m_NumPointsSlider;
	ofxButton m_TriangulateButton;
	ofxButton m_ResetButton;

	void numPointsChanged(int & numPoints);
	void isMonotoneChanged(bool & isMonotone);
	void triangulateButtonPressed();
	void resetButtonPressed();
	void updatePolygon();
};
