//
//  ofxMediaPipeFrame.h
//  ShowMeFace
//
//  Created by Nick Hardeman on 5/17/24.
//

#pragma once
#include "ofUtils.h"
#include "ofxMediaPipeFace.h"
#include "ofxMediaPipeHand.h"
#include "ofxMediaPipePose.h"
#if !defined(OFX_MEDIAPIPE_EXCLUDE_TRACKERS)
#include "ofxMediaPipeTracker.h"
#endif
#include "ofJson.h"

#if defined(OF_ADDON_HAS_OFX_OSC)
#include "ofxOsc.h"
#endif

namespace ofx::MediaPipe {
class Frame {
public:
	
	bool setup(ofJson& aJFrame );
	
	ofJson jsonify(const std::int64_t& atimestamp, const std::vector<std::shared_ptr<Face>>& afaces );
	ofJson jsonify(const std::int64_t& atimestamp, const std::vector<std::shared_ptr<Hand>>& ahands );
	ofJson jsonify(const std::int64_t& atimestamp, const std::vector<std::shared_ptr<Pose>>& aposes );
	
#if defined(OF_ADDON_HAS_OFX_OSC)
	ofxOscMessage getOscMessage( const std::uint64_t& aFrameNum, std::shared_ptr<Face> aface );
	ofxOscMessage getOscMessage( const std::uint64_t& aFrameNum, std::shared_ptr<Hand> ahand );
	ofxOscMessage getOscMessage( const std::uint64_t& aFrameNum, std::shared_ptr<Pose> apose );
	
	ofxOscMessage getOscMessageWorld( const std::uint64_t& aFrameNum, std::shared_ptr<Face> aface );
	ofxOscMessage getOscMessageWorld( const std::uint64_t& aFrameNum, std::shared_ptr<Hand> ahand );
	ofxOscMessage getOscMessageWorld( const std::uint64_t& aFrameNum, std::shared_ptr<Pose> apose );
	
	bool setup( ofxOscMessage& am, std::shared_ptr<TrackedObject>& aobject, bool abWorld);
#endif
	
	const std::int64_t& getTimestamp() { return timestamp;}
	const double getTimestampSeconds();
	const TrackedObject::TrackedObjectType& getType() { return mType; }
	
	std::vector<std::shared_ptr<Face>>& getFaces() {return mFaces;};
	std::vector<std::shared_ptr<Hand>>& getHands() {return mHands;};
	std::vector<std::shared_ptr<Pose>>& getPoses() {return mPoses;};
	
protected:
	void _serialize( ofJson& ajobj, const std::shared_ptr<TrackedObject>& aobject );
	ofJson _getJsonFromKeypoints( const std::shared_ptr<TrackedObject>& aobject );
#if defined(OF_ADDON_HAS_OFX_OSC)
	bool _setupOscMessage(ofxOscMessage& am, const std::uint64_t& aFrameNum, const std::shared_ptr<TrackedObject>& aobject, bool abWorld );
#endif
	std::int64_t timestamp = 0; // nanoseconds
	
	std::vector<std::shared_ptr<Face>> mFaces;
	std::vector<std::shared_ptr<Hand>> mHands;
	std::vector<std::shared_ptr<Pose>> mPoses;
	
	TrackedObject::TrackedObjectType mType = TrackedObject::HAND;
	
//	ofJson mJsonObj;
};
}
