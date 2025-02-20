//
//  ofxMediaPipeOscReceiver.h
//  TrackerMP
//
//  Created by Nick Hardeman on 10/14/24.
//

#pragma once
#include "ofxMediaPipeFrame.h"
#include "ofGraphicsBaseTypes.h"
#if defined(OF_ADDON_HAS_OFX_OSC)
#include "ofxOsc.h"
#endif

namespace ofx::MediaPipe {
class OscReceiver : public ofBaseDraws {
public:
	
	struct TrackedObjectRxInfo {
		ofRectangle outRect;
		bool bHasNewData = false;
	};
	
	OscReceiver();
	~OscReceiver();
	
	ofParameterGroup& getParams();
	float getTimeSinceReceivedData();
	
	void setup();
	void close();
	
	void draw(float x, float y, float w, float h) const override;
	
	float getWidth() const override;
	float getHeight() const override;
	
	ofRectangle getRect(const TrackedObject::TrackedObjectType& atype);
	ofRectangle getRect(const TrackedObject::TrackedObjectType& atype) const;
	
	int getVideoWidth();
	int getVideoHeight();
	
	bool hasNewData( const TrackedObject::TrackedObjectType& atype );
	bool hasNewFaceData();
	bool hasNewHandData();
	bool hasNewPoseData();
	
	std::vector< std::shared_ptr<ofx::MediaPipe::Face> >& getFaces();
	std::vector< std::shared_ptr<ofx::MediaPipe::Hand> >& getHands();
	std::vector< std::shared_ptr<ofx::MediaPipe::Pose> >& getPoses();
	
	std::vector< std::shared_ptr<ofx::MediaPipe::Face> >& getValidFaces();
	std::vector< std::shared_ptr<ofx::MediaPipe::Hand> >& getValidHands();
	std::vector< std::shared_ptr<ofx::MediaPipe::Pose> >& getValidPoses();
	
	TrackedObjectRxInfo& getTrackedObjectInfo(ofx::MediaPipe::TrackedObject::TrackedObjectType atype);
	
protected:
	void update( ofEventArgs& args );
	
	
	std::shared_ptr<Face> getFace( std::int32_t aid );
	std::shared_ptr<Hand> getHand( std::int32_t aid );
	std::shared_ptr<Pose> getPose( std::int32_t aid );
	
	bool isSetupForRx();
#if defined(OF_ADDON_HAS_OFX_OSC)
	std::shared_ptr<ofxOscReceiver> getReceiver() { return mOSCRX; }
#endif
	void deleteReceiver();
	void setupForRecieve(int port);
	
	void _checkEnabled();
	
	void _addAppEventListeners();
	void _removeEventListeners();
	
	void onPortParamChanged(int& aPort);
	
	ofParameterGroup mParams;
	
#if defined(OF_ADDON_HAS_OFX_OSC)
	std::shared_ptr<ofxOscReceiver> mOSCRX;
#endif
	ofParameter<int> mPort;
	ofParameter<bool> mBOscEnabled;
	ofParameter<std::string> mConnectionStatus;
	
	ofParameter<float> mShiftX, mShiftY, mScaleX, mScaleY;
	float mTimeSinceReceivedData = 10000.f;
	
	bool mBHasAppListeners = false;
	float mNextCheckTimef = 0.f;
	
	int mVideoWidth = 1920.f;
	int mVideoHeight = 1080.f;
	ofRectangle mVideoRect;
	
	std::unordered_map< ofx::MediaPipe::TrackedObject::TrackedObjectType, TrackedObjectRxInfo > mTInfoMap;
	
	std::vector< std::shared_ptr<ofx::MediaPipe::Face>> mFaces, mValidFaces;
	std::vector< std::shared_ptr<ofx::MediaPipe::Hand>> mHands, mValidHands;
	std::vector< std::shared_ptr<ofx::MediaPipe::Pose>> mPoses, mValidPoses;
	
	std::shared_ptr<Frame> mFrame;
	
};
}

