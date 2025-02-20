#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	
#if !defined(OF_ADDON_HAS_OFX_OSC)
	ofLogWarning("ofApp") << "Must define OF_ADDON_HAS_OFX_OSC as a preprocessor definition!";
#endif
	
	ofSetFrameRate( 60 );
	ofSetVerticalSync(false);
	
	
	gui.setup();
	gui.setPosition(24, 24);
	
	mMPReceiver = std::make_shared<ofx::MediaPipe::OscReceiver>();
	mMPReceiver->getParams().setName("ofxMediaPipeOscReceiver");
	gui.add( mMPReceiver->getParams() );
	gui.loadFromFile("settings.xml");
}

//--------------------------------------------------------------
void ofApp::update(){
	
}

//--------------------------------------------------------------
void ofApp::draw(){
	if( mMPReceiver->getVideoWidth() > 0 && mMPReceiver->getVideoHeight() > 0 ) {
		// create a rectangle representing the window dimensions
		ofRectangle windowRect( 0, 0, ofGetWidth(), ofGetHeight() );
		// create a rectangle representing the video dimensions
		ofRectangle videoRect( 0, 0, mMPReceiver->getVideoWidth(), mMPReceiver->getVideoHeight() );
		// fit the video rect to inside the window rect
		videoRect.scaleTo(windowRect);
		ofSetColor(255);
		ofPushMatrix(); {
			ofTranslate( videoRect.x, videoRect.y );
			ofScale( videoRect.getWidth() / (float)mMPReceiver->getVideoWidth() , videoRect.getHeight() / (float)mMPReceiver->getVideoHeight() );
			for( auto& face : mMPReceiver->getFaces() ) {
				face->drawOutlines(false);
			}
			for( auto& hand : mMPReceiver->getHands() ) {
				hand->drawOutlines(false);
			}
			for( auto& pose : mMPReceiver->getPoses() ) {
				pose->drawOutlines(false);
			}
		} ofPopMatrix();
	}
	
	
	gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){
	
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

