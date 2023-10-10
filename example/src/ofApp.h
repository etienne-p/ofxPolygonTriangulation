#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "TriangulateMonotone.h"
#include "SplitToMonotone.h"

class ofApp : public ofBaseApp {

public:
	void setup();
	void exit();
	void update();
	void draw();

private:
	bool m_IsMonotone;

	// GUI
	size_t m_NumPoints{ 9 };
	ofxPanel m_Gui;
	ofxToggle m_IsMonotoneToggle;
	ofxIntSlider m_NumPointsSlider;
	ofxButton m_SplitToMonotoneButton;
	ofxButton m_TriangulateButton;
	ofxButton m_ResetButton;
	void numPointsChanged(int& numPoints);
	void isMonotoneChanged(bool& isMonotone);
	void splitToMonotoneButtonPressed();
	void triangulateButtonPressed();
	void resetButtonPressed();

	DoublyConnectedEdgeList m_Dcel;
	SplitToMonotone m_SplitToMonotone;
	TriangulateMonotone m_TriangulateMonotone;

	vector<ofPolyline> m_Lines;
	ofMesh m_Mesh;

	// Cached cause there's an issue with ofSetRandomSeed afaik
	vector<ofColor> m_LineColors;
	// Can we use vec2?
	vector<glm::vec3> m_Vertices;
	vector<ofIndexType> m_Indices;
	stack<DoublyConnectedEdgeList::Face> m_FacesPendingTriangulation;

	void updatePolygon(ofPolyline& line);
};
