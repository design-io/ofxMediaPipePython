//
//  MediaPipeTracker.hpp
//  FaceDraw
//
//  Created by Theo on 2/8/24.
//

#pragma once
#if !defined(OFX_MEDIAPIPE_EXCLUDE_TRACKERS)
#include "ofxMediaPipeFace.h"
#include "ofxMediaPipeTracker.h"

namespace py = pybind11;

// Define a C++ wrapper for the FaceMeshDetector Python class
namespace ofx::MediaPipe {
class FaceTracker : public Tracker {
public:
	
	// https://developers.google.com/mediapipe/solutions/vision/face_landmarker
	class FaceSettings : public Tracker::Settings {
	public:
		FaceSettings() {}
		~FaceSettings() {};
		
		FaceSettings( const Tracker::Settings& asettings, bool aOutputFaceBlendshapes=false, bool aOutputFacialTransformationMatrices=false ) {
			runningMode = asettings.runningMode;
			maxNum = asettings.maxNum;
			minDetectionConfidence = asettings.minDetectionConfidence;
			minPresenceConfidence = asettings.minPresenceConfidence;
			minTrackingConfidence = asettings.minTrackingConfidence;
			filePath = asettings.filePath;
			outputFaceBlendshapes = aOutputFaceBlendshapes;
//			outputFacialTransformationMatrices = aOutputFacialTransformationMatrices;
		}
		bool outputFaceBlendshapes = false;
//		bool outputFacialTransformationMatrices = false;
	};
	
	FaceTracker();
	~FaceTracker();
	
	ofParameterGroup& getParams() override;
//	bool setup( int maxNumFaces = 1, float minDetectConf = 0.5, float minTrackConf = 0.5);
	bool setup( const Tracker::Settings& asettings );
	bool setup( const FaceSettings& asettings );
	
	void draw() override;
	void draw(float x, float y, float w, float h) const override;
	
	RunningMode getRunningMode() override { return mSettings.runningMode; }
	TrackerType getTrackerType() override { return TrackedObject::FACE; };
	std::vector< std::shared_ptr<Face> >& getFaces();
	
private:
	void _update() override;
	void _process_landmark_results( py::object& aresults, int aTimestamp) override;
	
	void _matchFaces( std::vector<std::shared_ptr<Face>>& aIncomingFaces, std::vector<std::shared_ptr<Face>>& aFaces );
	
	ofParameter<bool> mBDrawIrises;
//	ofParameter<float> mMaxDistToMatch;
	ofParameter<float> mMinScoreForActiveBlendshape;
	ofParameter<float> mPosSmoothing;
	
	FaceSettings mSettings;
	
	std::vector< std::shared_ptr<Face> > mThreadedFaces;
	std::vector< std::shared_ptr<Face> > mFaces;
	
};
}
#endif
