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

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
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
	static void createPolygonRandom(vector<glm::vec3>& points, size_t numPoints);
	static void createPolygonRandomMonotone(vector<glm::vec3>& points, size_t numPoints);
};
