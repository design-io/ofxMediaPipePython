#include "ofApp.h"

// setting this to avoid typing ofx::MediaPipe::HandTracker
// now we can just use HandTracker
using namespace ofx::MediaPipe;

//--------------------------------------------------------------
void ofApp::setup(){
	
	ofSetFrameRate( 60 );
	ofSetVerticalSync(false);
	
	mGrabber.setup(1280, 720);
	
	auto runMode = Tracker::MODE_VIDEO;
	// Tracker::MODE_LIVE_STREAM uses threads, but can be a bit slower when talking between python GIL threads and C++.
	// Helpful when running multiple trackers and not bogging down the entire app.
	runMode = Tracker::MODE_LIVE_STREAM;
	
	// setup the hand tracker //
	handTracker = make_shared<HandTracker>();
	HandTracker::HandSettings hsettings;
	hsettings.runningMode = runMode;
	hsettings.maxNum = 2;
	handTracker->setup( hsettings );
	
	faceTracker = make_shared<FaceTracker>();
	ofx::MediaPipe::FaceTracker::FaceSettings fsettings;
	fsettings.maxNum = 2;
	fsettings.minDetectionConfidence = 0.25f;
	fsettings.minPresenceConfidence = 0.25f;
	fsettings.minTrackingConfidence = 0.25f;
	fsettings.runningMode = runMode;
	faceTracker->setup(fsettings);
	
	poseTracker = make_shared<ofx::MediaPipe::PoseTracker>();
	ofx::MediaPipe::PoseTracker::PoseSettings psettings;//({2, filePath}, false );
	psettings.maxNum = 2;
	psettings.runningMode = runMode;
	poseTracker->setup( psettings );
}

//--------------------------------------------------------------
void ofApp::update(){
	mGrabber.update();
	if( mGrabber.isInitialized() && mGrabber.isFrameNew() ) {
		mVideoFps.newFrame();
		mVideoPixels = mGrabber.getPixels();
		mVideoPixels.mirror( false, true );
		handTracker->process(mVideoPixels);
		faceTracker->process(mVideoPixels);
		poseTracker->process(mVideoPixels);
		mVideoTexture.loadData(mVideoPixels);
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	if( mVideoTexture.getWidth() > 0 && mVideoTexture.getHeight() > 0 ) {
		// create a rectangle representing the window dimensions
		ofRectangle windowRect( 0, 0, ofGetWidth(), ofGetHeight() );
		// create a rectangle representing the video dimensions
		ofRectangle videoRect( 0, 0, mVideoTexture.getWidth(), mVideoTexture.getHeight() );
		// fit the video rect to inside the window rect
		videoRect.scaleTo(windowRect);
		ofSetColor(255);
		mVideoTexture.draw(videoRect);
		ofPushMatrix(); {
			ofTranslate( videoRect.x, videoRect.y );
			ofScale( videoRect.getWidth() / mVideoTexture.getWidth(), videoRect.getHeight() / mVideoTexture.getHeight() );
			faceTracker->draw();
			handTracker->draw();
			poseTracker->draw();
		} ofPopMatrix();
		
		stringstream ss;
		ss << "App FPS: " << ofGetFrameRate();
		ss << std::endl << "Video FPS: " << mVideoFps.getFps();
		ss << std::endl << "Hand Tracker FPS: " << handTracker->getFps();
		ss << std::endl << "Face Tracker FPS: " << faceTracker->getFps();
		ss << std::endl << "Pose Tracker FPS: " << poseTracker->getFps();
		
		ofDrawBitmapStringHighlight(ss.str(), 24, 24 );
	}
}

//--------------------------------------------------------------
void ofApp::exit(){
	// remove instances of trackers
	handTracker.reset();
	poseTracker.reset();
	faceTracker.reset();
	// once all of the tracker instances have been removed, then clean up interpreter 
	// clean up the interpreter //
	Tracker::PyShutdown();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
	
}

