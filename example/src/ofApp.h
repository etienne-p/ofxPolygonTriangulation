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
	bool m_IsWireFrame;

	// GUI
	size_t m_NumPoints { 9 };
	ofxPanel m_Gui;
	ofxIntSlider m_NumPointsSlider;
	ofxToggle m_IsMonotoneToggle;
	ofxToggle m_IsWireFrameToggle;
	ofxButton m_SplitToMonotoneButton;
	ofxButton m_TriangulateButton;
	ofxButton m_ResetButton;
	void numPointsChanged(int & numPoints);
	void isMonotoneChanged(bool & isMonotone);
	void isWireFrameChanged(bool & isWireFrame);
	void splitToMonotoneButtonPressed();
	void triangulateButtonPressed();
	void resetButtonPressed();

	ofDoublyConnectedEdgeList m_Dcel;
	ofSplitToMonotone m_SplitToMonotone;
	ofPolygonTriangulation m_Triangulation;

	vector<ofPolyline> m_Lines;
	ofMesh m_Mesh;

	// Cached cause there's an issue with ofSetRandomSeed afaik.
	vector<ofColor> m_LineColors;
	vector<glm::vec3> m_Vertices;
	vector<ofIndexType> m_Indices;
	stack<ofDoublyConnectedEdgeList::Face> m_FacesPendingTriangulation;

	void updatePolygon(ofPolyline & line);
};
