//
//  ofxMediaPipeOscSender.h
//  TrackerMP
//
//  Created by Nick Hardeman on 10/14/24.
//

#pragma once
#include "ofxMediaPipeFrame.h"
#if defined(OF_ADDON_HAS_OFX_OSC)
#include "ofxOsc.h"
#endif

namespace ofx::MediaPipe {
class OscSender {
public:
	
	OscSender();
	~OscSender();
	
	ofParameterGroup& getParams();
	void setup();
	void setVideoWidth( int aw );
	void setVideoHeight( int ah );
	
//	void setWidth( const TrackedObject::TrackedObjectType& atype, int aw );
//	void setHeight( const TrackedObject::TrackedObjectType& atype, int ah );
	void setOutRect( const TrackedObject::TrackedObjectType& atype, ofRectangle arect );
	
	template<typename T>
	void send( const std::vector<std::shared_ptr<T>>& aobjs ) {
#if !defined(OF_ADDON_HAS_OFX_OSC)
		ofLogWarning("ofx::MediaPipe::OscSender") << "Make sure to define OF_ADDON_HAS_OFX_OSC and include the ofxOsc addon!";
		return;
		#endif
		
		if( !mFrame ) {
			mFrame = std::make_shared<Frame>();
		}
		
		#if defined(OF_ADDON_HAS_OFX_OSC)
		if( mOSCSend ) {
			auto frameNum = ofGetFrameNum();
//			unsigned int counter = 0;
			for( const auto& obj : aobjs ) {
//				ofLogNotice("ofx::MediaPipe::OscSender") << " getting osx message for " << counter;
				auto m = mFrame->getOscMessage(frameNum, obj);
				mOSCSend->sendMessage( m );
				
				auto wm = mFrame->getOscMessageWorld(frameNum, obj);
				mOSCSend->sendMessage( wm );
				
//				counter++;
			}
		}
		#endif
	}
	
#if defined(OF_ADDON_HAS_OFX_OSC)
	std::shared_ptr<ofxOscSender> getSender() { return mOSCSend; }
#endif
	bool isSetupForSend();
	
	ofParameter<int>& getPortParam() { return mBroadcastPort;}
	
protected:
	
	void setupForSend(std::string ipAddress, int port);
	void deleteSender();
	
	void update( ofEventArgs& args );
	
	void onIpParamChanged(std::string& aIp);
	void onPortParamChanged(int& aPort);
	
	void _checkEnabled();
	void _addAppEventListeners();
	void _remoteEventListeners();
	
	ofParameterGroup mParams;
	
#if defined(OF_ADDON_HAS_OFX_OSC)
	std::shared_ptr<ofxOscSender> mOSCSend;
#endif
	ofParameter<std::string> mBroadcastIp;
	ofParameter<bool> mBOscEnabled;
	ofParameter<int> mBroadcastPort;
	ofParameter<float> mHeartbeatFreq;
	float mHeartBeatDelta = 0.0f;
	float mNextCheckOscSenderTimef = 0.0f;
	
	std::shared_ptr<Frame> mFrame;

	
	bool bHasEventListeners = false;
	
	int mVideoWidth = 0;
	int mVideoHeight = 0;
	
	std::unordered_map< ofx::MediaPipe::TrackedObject::TrackedObjectType, ofRectangle > mOutRectMap;
};
}
