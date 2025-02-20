#pragma once
#include "ofMain.h"
#include "ofxMediaPipeOscReceiver.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp{
public:
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;

	void keyPressed(int key) override;
	void keyReleased(int key) override;
	void mouseMoved(int x, int y ) override;
	void mouseDragged(int x, int y, int button) override;
	void mousePressed(int x, int y, int button) override;
	void mouseReleased(int x, int y, int button) override;
	void mouseScrolled(int x, int y, float scrollX, float scrollY) override;
	void mouseEntered(int x, int y) override;
	void mouseExited(int x, int y) override;
	void windowResized(int w, int h) override;
	
	
	
	std::shared_ptr<ofx::MediaPipe::OscReceiver> mMPReceiver;
	
	ofFpsCounter mVideoFps;
	
	ofxPanel gui;
};
