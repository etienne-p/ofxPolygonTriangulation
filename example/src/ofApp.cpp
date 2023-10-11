#include "ofApp.h"
#include "ofPolygonUtility.h"
#include "ofTriangulatemonotone.h"
#include <cassert>
#include <random>

void ofApp::setup() {
	ofSetVerticalSync(true);

	m_NumPointsSlider.addListener(this, &ofApp::numPointsChanged);
	m_IsMonotoneToggle.addListener(this, &ofApp::isMonotoneChanged);
	m_TriangulateButton.addListener(this, &ofApp::triangulateButtonPressed);
	m_ResetButton.addListener(this, &ofApp::resetButtonPressed);

	m_Gui.setup();
	m_Gui.add(m_NumPointsSlider.setup("Num Points", m_NumPoints, 2, 64));
	m_Gui.add(m_IsMonotoneToggle.setup("Is Monotone", m_IsMonotone));
	m_Gui.add(m_TriangulateButton.setup("Triangulate"));
	m_Gui.add(m_ResetButton.setup("Reset"));
}

void ofApp::exit() {
	m_NumPointsSlider.removeListener(this, &ofApp::numPointsChanged);
	m_IsMonotoneToggle.removeListener(this, &ofApp::isMonotoneChanged);
	m_TriangulateButton.removeListener(this, &ofApp::triangulateButtonPressed);
	m_ResetButton.removeListener(this, &ofApp::resetButtonPressed);
}

void ofApp::update() { }

void ofApp::draw() {
	auto width = ofGetWidth();
	auto height = ofGetHeight();

	// TODO Replace by some scope mapping a rect coords to screen.
	ofPushMatrix();
	ofTranslate(glm::vec3(width / 2.0f, height / 2.0f, 0));
	ofScale(glm::vec3(width / 2.0f, height / 2.0f, 1.0f));

	ofSetColor(ofColor::aquamarine);
	m_Mesh.drawWireframe();

	ofSetColor(ofColor::indianRed);
	m_Line.draw();

	ofPopMatrix();

	m_Gui.draw();
}

void ofApp::numPointsChanged(int & numPoints) {
	m_NumPoints = numPoints;
	updatePolygon();
	m_Mesh.clear();
}

void ofApp::isMonotoneChanged(bool & isMonotone) {
	m_IsMonotone = isMonotone;
	updatePolygon();
	m_Mesh.clear();
}

void ofApp::triangulateButtonPressed() {
	if (m_Vertices.empty()) {
		updatePolygon();
	}

	m_Dcel.initializeFromCCWVertices(m_Vertices);
	m_PolygonTriangulation.execute(m_Dcel);
	m_Dcel.extractTriangles(m_Vertices, m_Indices);

	m_Mesh.clear();
	m_Mesh.addVertices(m_Vertices);
	m_Mesh.addIndices(m_Indices);
}

void ofApp::resetButtonPressed() {
	m_Line.clear();
	m_Mesh.clear();
}

void ofApp::updatePolygon() {
	m_Vertices.resize(m_NumPoints);

	if (m_IsMonotone) {
		ofPolygonUtility::createPolygonRandomMonotone(m_Vertices, m_NumPoints);
	} else {
		ofPolygonUtility::createPolygonRandom(m_Vertices, m_NumPoints);
	}

	m_Line.clear();
	m_Line.addVertices(m_Vertices);
	m_Line.setClosed(true);
}
