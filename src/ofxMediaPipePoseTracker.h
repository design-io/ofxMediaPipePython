//
//  ofxMediaPipePoseTracker.h
//  PoseTrackerDemo
//
//  Created by Nick Hardeman on 4/30/24.
//

#pragma once

#if !defined(OFX_MEDIAPIPE_EXCLUDE_TRACKERS)
#include "ofxMediaPipeTracker.h"
#include "ofxMediaPipePose.h"
#include "ofTexture.h"

namespace ofx::MediaPipe {
class PoseTracker : public Tracker {
public:
	
	PoseTracker();
	~PoseTracker();
	
	// https://developers.google.com/mediapipe/solutions/vision/pose_landmarker
	class PoseSettings : public Tracker::Settings {
	public:
		PoseSettings() {};
		~PoseSettings() {};
		PoseSettings( const Tracker::Settings& asettings, bool bOutSegmentationMasks=false ) {
			runningMode = asettings.runningMode;
			maxNum = asettings.maxNum;
			minDetectionConfidence = asettings.minDetectionConfidence;
			minPresenceConfidence = asettings.minPresenceConfidence;
			minTrackingConfidence = asettings.minTrackingConfidence;
			filePath = asettings.filePath;
			outputSegmentationMasks = bOutSegmentationMasks;
		}
		bool outputSegmentationMasks = false;
	};
	
	ofParameterGroup& getParams() override;
	bool setup(const Tracker::Settings& asettings);
	bool setup(const PoseSettings& asettings);
	
	void draw() override;
	void draw(float x, float y, float w, float h) const override;
	
	
	RunningMode getRunningMode() override { return mSettings.runningMode; }
	TrackerType getTrackerType() override { return TrackedObject::POSE; };
	
	std::vector< std::shared_ptr<Pose> >& getPoses() { return mPoses; }
	ofFloatPixels& getMaskPixels();
	ofTexture& getMaskTexture();
	int getNumRawPoses() { return mRawPoses.size(); }
	
protected:
	void _update() override;
	void _process_landmark_results( py::object& aresults, int aTimestamp) override;
	void _matchPoses( std::vector< std::shared_ptr<Pose>>& aIncomingPoses, std::vector< std::shared_ptr<Pose>>& aPoses );
	
	bool _areFeetAboveHips( std::shared_ptr<Pose>& apose );
	bool _areFeetAboveHead( std::shared_ptr<Pose>& apose );
	bool _isFaceDirectionUp( std::shared_ptr<Pose>& apose );
	float _getMatchScore( std::shared_ptr<Pose>& aInPose, std::shared_ptr<Pose>& apose );
	int _hasBetterMatchToExistingPose( std::shared_ptr<Pose>& aInPose1, std::shared_ptr<Pose>& aInPose2, std::vector< std::shared_ptr<Pose>>& aPoses );
	
	PoseSettings mSettings;
	
	ofParameter<bool> mBDrawMask;
	ofParameter<float> mPosSmoothing;
	
	std::vector< std::shared_ptr<Pose> > mThreadedPoses;
	std::vector< std::shared_ptr<Pose> > mPoses, mRawPoses;
	
	int mPixWidth = 0;
	int mPixHeight = 0;
//	std::vector<uint8_t> mPixelData;
	std::vector<float> mPixelData;
	
	ofFloatPixels mMaskPix;
	bool mBTexDirty = false;
	ofTexture mMaskTexture;

	
	
};
}
#endif
