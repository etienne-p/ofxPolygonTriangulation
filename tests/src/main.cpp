#include "ofDoublyConnectedEdgeList.h"
#include "ofAppNoWindow.h"
#include "ofMain.h"
#include "ofxUnitTests.h"

class ofApp : public ofxUnitTestsApp {
private:
	ofDoublyConnectedEdgeList m_Dcel;

public:
	void run() {
	}
};

int main() {
	ofInit();
	auto window = std::make_shared<ofAppNoWindow>();
	auto app = std::make_shared<ofApp>();
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(window, app);
	return ofRunMainLoop();
}