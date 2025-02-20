#include "ofApp.h"

// setting this to avoid typing ofx::MediaPipe::HandTracker
// now we can just use HandTracker
using namespace ofx::MediaPipe;

//--------------------------------------------------------------
void ofApp::setup(){
	
#if !defined(OF_ADDON_HAS_OFX_OSC)
	ofLogWarning("ofApp") << "Must define OF_ADDON_HAS_OFX_OSC as a preprocessor definition!";
#endif
	
	ofSetFrameRate( 60 );
	ofSetVerticalSync(false);
	
	mGrabber.setup(1280, 720 );
	
	auto runMode = Tracker::MODE_VIDEO;
	// also try Tracker::MODE_LIVE_STREAM which uses threads, but can be a bit slower when talking between python GIL threads and C++. Can be helpful when running multiple trackers and not bogging down the entire app.
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
	
	poseTracker = make_shared<PoseTracker>();
	ofx::MediaPipe::PoseTracker::PoseSettings psettings;//({2, filePath}, false );
	psettings.maxNum = 2;
	psettings.runningMode = runMode;
	poseTracker->setup( psettings );
	
	mOscSender = make_shared<ofx::MediaPipe::OscSender>();
	gui.setup();
	gui.setPosition(24, 128);
	gui.add( mOscSender->getParams() );
	gui.add( handTracker->getParams() );
	gui.add( faceTracker->getParams() );
	gui.add( poseTracker->getParams() );
	
	
	gui.loadFromFile("settings.xml");
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
		
		mOscSender->setVideoWidth(mVideoTexture.getWidth());
		mOscSender->setVideoHeight(mVideoTexture.getHeight());
		
//		ofRectangle videoRect( 0, 0, mVideoTexture.getWidth(), mVideoTexture.getHeight() );
//		
//		mOscSender->setOutRect(TrackedObject::HAND, videoRect );
//		mOscSender->setOutRect(TrackedObject::FACE, videoRect );
//		mOscSender->setOutRect(TrackedObject::POSE, videoRect );
	}
	
	if( handTracker->hasNewData() ) {
		mOscSender->send<ofx::MediaPipe::Hand>( handTracker->getHands() );
	}
	if( faceTracker->hasNewData() ) {
		mOscSender->send<ofx::MediaPipe::Face>( faceTracker->getFaces() );
	}
	if( poseTracker->hasNewData() ) {
		mOscSender->send<ofx::MediaPipe::Pose>( poseTracker->getPoses() );
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
	
	
	gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){
	handTracker.reset();
	poseTracker.reset();
	faceTracker.reset();
	
	if (Py_IsInitialized()) {
		ofLogNotice("ofApp") << "finalize pybind interpreter.";
		try {
			//py::gil_scoped_acquire acquire;
			try {
				py::finalize_interpreter();
			} catch (py::error_already_set& e) {
				std::cerr << "Python error: " << e.what() << std::endl;
			}
		} catch (const std::exception& e) {
			ofLogError("ERROR with scoped acquire") << e.what();
		} catch (...) {
			ofLogError("ERROR with scoped acquire");
		}
	}
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

