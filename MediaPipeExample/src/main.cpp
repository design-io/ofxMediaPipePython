#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){

	//Use ofGLFWWindowSettings for more options like multi-monitor fullscreen
	ofGLWindowSettings settings;
	settings.setSize(1400, 800);
	settings.windowMode = OF_WINDOW; //can also be OF_FULLSCREEN
	settings.setGLVersion(3, 2);
	
	settings.title = "ofxMediaPipe Example";

	auto window = ofCreateWindow(settings);

	ofRunApp(window, std::make_shared<ofApp>());
	ofRunMainLoop();

}
