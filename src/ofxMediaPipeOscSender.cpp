//
//  ofxMediaPipeOscSender.cpp
//  TrackerMP
//
//  Created by Nick Hardeman on 10/14/24.
//

#include "ofxMediaPipeOscSender.h"

using namespace ofx::MediaPipe;

using std::make_shared;
using std::shared_ptr;
using std::vector;
using std::string;

//--------------------------------------------------------------
OscSender::OscSender() {
	mParams.setName("ofxMediaPipeOscSender");
#if defined(OF_ADDON_HAS_OFX_OSC)
	mBOscEnabled.set("Enabled", true);
	mBroadcastIp.set("Broadcast IP", "127.0.0.1");
	mBroadcastPort.set("Port", 9009, 8000, 12000);
	mHeartbeatFreq.set("HeartBeatFreq", 1.0, 0.0, 5.0);
#endif
}

//--------------------------------------------------------------
OscSender::~OscSender() {
	_remoteEventListeners();
}

//--------------------------------------------------------------
ofParameterGroup& OscSender::getParams() {

	if( mParams.size() < 1 ) {
#if defined(OF_ADDON_HAS_OFX_OSC)
		mParams.add(mBOscEnabled);
		mParams.add(mBroadcastIp);
		mParams.add(mBroadcastPort);
		mParams.add(mHeartbeatFreq);
#endif
	}

	_addAppEventListeners();
	return mParams;
}

//--------------------------------------------------------------
void OscSender::setup() {
	getParams();
}

//--------------------------------------------------------------
void OscSender::setVideoWidth( int aw ) {
	mVideoWidth = aw;
	
	if( mVideoWidth > 0 && mVideoHeight > 0 ) {
		if( mOutRectMap.count(TrackedObject::FACE) < 1 ) {
			setOutRect(TrackedObject::FACE, ofRectangle(0,0,mVideoWidth,mVideoHeight));
		}
		if( mOutRectMap.count(TrackedObject::HAND) < 1 ) {
			setOutRect(TrackedObject::HAND, ofRectangle(0,0,mVideoWidth,mVideoHeight));
		}
		if( mOutRectMap.count(TrackedObject::POSE) < 1 ) {
			setOutRect(TrackedObject::POSE, ofRectangle(0,0,mVideoWidth,mVideoHeight));
		}
	}
}

//--------------------------------------------------------------
void OscSender::setVideoHeight( int ah ) {
	mVideoHeight = ah;
	if( mVideoWidth > 0 && mVideoHeight > 0 ) {
		if( mOutRectMap.count(TrackedObject::FACE) < 1 ) {
			setOutRect(TrackedObject::FACE, ofRectangle(0,0,mVideoWidth,mVideoHeight));
		}
		if( mOutRectMap.count(TrackedObject::HAND) < 1 ) {
			setOutRect(TrackedObject::HAND, ofRectangle(0,0,mVideoWidth,mVideoHeight));
		}
		if( mOutRectMap.count(TrackedObject::POSE) < 1 ) {
			setOutRect(TrackedObject::POSE, ofRectangle(0,0,mVideoWidth,mVideoHeight));
		}
	}
}

//--------------------------------------------------------------
void OscSender::setOutRect( const TrackedObject::TrackedObjectType& atype, ofRectangle arect ) {
	mOutRectMap[atype] = arect;
}

//--------------------------------------------------------------
void OscSender::update( ofEventArgs& args ) {
	_checkEnabled();
	
#if defined(OF_ADDON_HAS_OFX_OSC)
	float deltaTime = ofClamp(ofGetLastFrameTime(), 1.f / 2500.f, 1.f / 5.f );
	
	if (mHeartbeatFreq > 0.0f) {
		mHeartBeatDelta += deltaTime;
		if (mHeartBeatDelta >= mHeartbeatFreq) {
			mHeartBeatDelta = 0.0f;
			
			if( mOSCSend ) {
				ofxOscMessage hm;
				hm.setAddress("/ofxmp/frame/video");
				hm.addIntArg(0);
				hm.addIntArg(0);
				hm.addIntArg(mVideoWidth);
				hm.addIntArg(mVideoHeight);
				mOSCSend->sendMessage(hm);
				
				if( mOutRectMap.count(TrackedObject::FACE) > 0 ) {
					ofxOscMessage hm;
					hm.setAddress("/ofxmp/frame/faces");
					hm.addIntArg(mOutRectMap[TrackedObject::FACE].x);
					hm.addIntArg(mOutRectMap[TrackedObject::FACE].y);
					hm.addIntArg(mOutRectMap[TrackedObject::FACE].width);
					hm.addIntArg(mOutRectMap[TrackedObject::FACE].height);
					mOSCSend->sendMessage(hm);
				}
				
				if( mOutRectMap.count(TrackedObject::HAND) > 0 ) {
					ofxOscMessage hm;
					hm.setAddress("/ofxmp/frame/hands");
					hm.addIntArg(mOutRectMap[TrackedObject::HAND].x);
					hm.addIntArg(mOutRectMap[TrackedObject::HAND].y);
					hm.addIntArg(mOutRectMap[TrackedObject::HAND].width);
					hm.addIntArg(mOutRectMap[TrackedObject::HAND].height);
					mOSCSend->sendMessage(hm);
				}
				
				if( mOutRectMap.count(TrackedObject::POSE) > 0 ) {
					ofxOscMessage hm;
					hm.setAddress("/ofxmp/frame/poses");
					hm.addIntArg(mOutRectMap[TrackedObject::POSE].x);
					hm.addIntArg(mOutRectMap[TrackedObject::POSE].y);
					hm.addIntArg(mOutRectMap[TrackedObject::POSE].width);
					hm.addIntArg(mOutRectMap[TrackedObject::POSE].height);
					mOSCSend->sendMessage(hm);
				}
			}
		}
	} else {
		mHeartBeatDelta = 0.0;
	}
	
	#endif
}

//--------------------------------------------------------------
void OscSender::setupForSend(std::string ipAddress, int port) {
#if defined(OF_ADDON_HAS_OFX_OSC)
	if( mOSCSend ) {deleteSender();}
	_addAppEventListeners();
	mOSCSend = make_shared<ofxOscSender>();
	ofxOscSenderSettings settings;
	settings.host = ipAddress;
	settings.port = port;
	
	mOSCSend->setup(settings);
	
	// force send a heartbeat data on update 
	mHeartBeatDelta = 9999.f;
#endif
}

//--------------------------------------------------------------
bool OscSender::isSetupForSend() {
#if defined(OF_ADDON_HAS_OFX_OSC)
	if (mOSCSend) {
		return true;
	}
#endif
	return false;
}

//--------------------------------------------------------------
void OscSender::deleteSender() {
#if defined(OF_ADDON_HAS_OFX_OSC)
	if(mOSCSend) {
		mOSCSend->clear();
		mOSCSend.reset();
	}
#endif
}


//--------------------------------------------------------------
void OscSender::onIpParamChanged(string& aIp) {
#if defined(OF_ADDON_HAS_OFX_OSC)
	if (isSetupForSend() && aIp != getSender()->getHost()) {
		deleteSender();
	}
#endif
	mNextCheckOscSenderTimef = ofGetElapsedTimef() + 1.f;
}

//--------------------------------------------------------------
void OscSender::onPortParamChanged(int& aPort) {
#if defined(OF_ADDON_HAS_OFX_OSC)
	if (isSetupForSend()) {
		auto sender = getSender();
		if (mBroadcastIp.get() != sender->getHost() || aPort != sender->getPort()) {
			deleteSender();
		}
	}
#endif
	mNextCheckOscSenderTimef = ofGetElapsedTimef() + 2.f;
}


//---------------------------------------------
void OscSender::_checkEnabled() {
#if defined(OF_ADDON_HAS_OFX_OSC)
	if (mBOscEnabled) {
		float etimef = ofGetElapsedTimef();
		if (etimef > mNextCheckOscSenderTimef) {
			if (!isSetupForSend()) {
				setupForSend(mBroadcastIp, mBroadcastPort);
				ofLogNotice("ofx::MediaPipe::OscSender") << " _checkEnabled: " << mBroadcastIp << " port " << mBroadcastPort << std::endl;
			}
			mNextCheckOscSenderTimef = etimef + 1.f;
		}
	} else {
		if (isSetupForSend()) {
			deleteSender();
		}
	}
#endif
}


//---------------------------------------------
void OscSender::_addAppEventListeners() {
	if (!bHasEventListeners) {
		ofAddListener(ofEvents().update, this, &OscSender::update, OF_EVENT_ORDER_BEFORE_APP );
		mBroadcastIp.addListener(this, &OscSender::onIpParamChanged);
		mBroadcastPort.addListener(this, &OscSender::onPortParamChanged);
	}
	bHasEventListeners = true;
}

//---------------------------------------------
void OscSender::_remoteEventListeners() {
	if (bHasEventListeners) {
		ofRemoveListener(ofEvents().update, this, &OscSender::update, OF_EVENT_ORDER_BEFORE_APP );
		mBroadcastIp.removeListener(this, &OscSender::onIpParamChanged);
		mBroadcastPort.removeListener(this, &OscSender::onPortParamChanged);
	}
	bHasEventListeners = false;
}
