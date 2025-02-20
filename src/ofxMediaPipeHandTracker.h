//
//  MediaPipeHandTracker.hpp
//  FaceDraw
//
//  Created by Theo on 2/8/24.
//

#pragma once
#if !defined(OFX_MEDIAPIPE_EXCLUDE_TRACKERS)
#include "ofxMediaPipeTracker.h"
#include "ofxMediaPipeHand.h"

namespace py = pybind11;

namespace ofx::MediaPipe {
class HandTracker : public Tracker {
public:
	HandTracker();
	~HandTracker();
	
	//https://developers.google.com/mediapipe/solutions/vision/hand_landmarker
	class HandSettings : public Tracker::Settings {
	public:
		HandSettings() {};
		~HandSettings() {};
		HandSettings( const Tracker::Settings& asettings ) {
			runningMode = asettings.runningMode;
			maxNum = asettings.maxNum;
			minDetectionConfidence = asettings.minDetectionConfidence;
			minPresenceConfidence = asettings.minPresenceConfidence;
			minTrackingConfidence = asettings.minTrackingConfidence;
			filePath = asettings.filePath;
		}
	};
	
	ofParameterGroup& getParams() override;
//	bool setup(Tracker::RunningMode aMode, int numHands = 4, float minDetectConf = 0.5, float minTrackConf = 0.5);
	bool setup( const Tracker::Settings& asettings);
	bool setup(const HandSettings& asettings);
	
	void draw() override;
	void draw(float x, float y, float w, float h) const override;
	
	RunningMode getRunningMode() override { return mSettings.runningMode; }
	TrackerType getTrackerType() override { return TrackedObject::HAND; };
	std::vector< std::shared_ptr<Hand> >& getHands();
	
protected:
	void _update() override;
	void _matchHands( std::vector<std::shared_ptr<Hand>>& aIncomingHands, std::vector< std::shared_ptr<Hand>> & aHands );
	
	ofParameter<float> mMaxDegForOpenFinger;
//	ofParameter<float> mMaxDistToMatch;
	ofParameter<bool> mBDrawFingers, mBDrawFingerInfo;
	ofParameter<float> mPosSmoothing;
	
	void _process_landmark_results( py::object& aresults, int aTimestamp) override;
	
	std::vector< std::shared_ptr<Hand> > mThreadedHands;
	std::vector< std::shared_ptr<Hand> > mHands;
	
	HandSettings mSettings;
	
};

}
#endif
