//
//  ofxMediaPipeTracker.h
//  HandShapeController
//
//  Created by Nick Hardeman on 4/25/24.
//

#pragma once

#include "ofVectorMath.h"
#include "ofRectangle.h"
#include "ofEvents.h"
#include "ofPixels.h"
#include "ofParameter.h"
#include <condition_variable>

#if !defined(OFX_MEDIAPIPE_EXCLUDE_TRACKERS)

#include "ofxMediaPipeTrackedObject.h"
#include "ofFpsCounter.h"

#include <pybind11/embed.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

namespace ofx::MediaPipe {
class Tracker {
public:
	
	typedef TrackedObject::TrackedObjectType TrackerType;
	
	enum RunningMode {
		MODE_IMAGE=0,
		MODE_VIDEO,
		MODE_LIVE_STREAM,
		MODE_OF_VIDEO_THREAD
	};
	
	class Settings {
	public:
		Settings() {}
		~Settings() {}
		
		Settings(RunningMode amode, int amaxNum, float aminDetectionConfidence=0.5f, float aminPresenceConfidence=0.5f, float aminTrackingConfidence=0.5f, of::filesystem::path aFilePath = "" ) {
			runningMode = amode;
			maxNum = amaxNum;
			minDetectionConfidence = aminDetectionConfidence;
			minPresenceConfidence = aminPresenceConfidence;
			minTrackingConfidence = aminTrackingConfidence;
			filePath = aFilePath;
		};
		
		Settings(int amaxNum, of::filesystem::path aFilePath) {
			maxNum = amaxNum;
			filePath = aFilePath;
		};
		
		Settings( of::filesystem::path aFilePath ) {
			filePath = aFilePath;
		};
		
		Settings( const Settings& aother ) {
			runningMode = aother.runningMode;
			maxNum = aother.maxNum;
			minDetectionConfidence = aother.minDetectionConfidence;
			minPresenceConfidence = aother.minPresenceConfidence;
			minTrackingConfidence = aother.minTrackingConfidence;
			filePath = aother.filePath;
		}
		
		Tracker::RunningMode runningMode = Tracker::MODE_VIDEO;
		int maxNum = 2;
		float minDetectionConfidence = 0.5f;
		float minPresenceConfidence = 0.5f;
		float minTrackingConfidence = 0.5f;
		of::filesystem::path filePath { "" };
		
	};
	
	static std::string sGetStringForRunningMode( RunningMode amode ) {
		std::string rmodeStr = "IMAGE";
		if( amode == Tracker::MODE_VIDEO || amode == Tracker::MODE_OF_VIDEO_THREAD ) {
			rmodeStr = "VIDEO";
		} else if( amode == Tracker::MODE_LIVE_STREAM ) {
			rmodeStr = "LIVE_STREAM";
		}
		return rmodeStr;
	};
	
	static bool PyShutdown();
	virtual ofParameterGroup& getParams() = 0;
	
	virtual void release();
	
	void update( ofEventArgs& args );
	virtual void draw() = 0;
	virtual void draw(float x, float y, float w, float h) const = 0;
	
	bool isSetup() { return mBSetup; }
	virtual RunningMode getRunningMode() = 0;
	virtual TrackerType getTrackerType() = 0;
	
	std::string getTrackerTypeAsString() { return TrackedObject::sGetTypeAsString(getTrackerType()); }
	
	virtual void process( const ofPixels& apix );
	
	bool hasNewData() { return mBHasNewData;}
	void setOutRect(const ofRectangle& arect) { mOutRect = arect; }
	
	ofRectangle& getSrcRect() { return mSrcRect; }
	ofRectangle& getOutRect() { return mOutRect; }
	
	ofFpsCounter& getFpsCounter() { return mFpsCounter; }
	double getFps() { return mFpsCounter.getFps(); }
	
	void setGuiPrefix( std::string apre ) {mGuiPrefix=apre;}
	
	void setDrawPointSize(float af) {mDrawPointSize=af;}
	
protected:
	void _onExit( ofEventArgs& args );
	
	virtual void _update() = 0;
	void _process_image(const ofPixels& apix);
	virtual void _process_landmark_results( py::object& aresults, int aTimestamp) = 0;
	
	void _process_results_callback(py::object& aresults, py::object& aMpImage, int aTimestamp);
	std::function<void(py::object& aresults, py::object& aMpImage, int aTimestamp)> process_results_lambda = nullptr;
	
	py::object _getMpImageFromPixels( const ofPixels& apix );
	
	void _startVideoPixThread();
	void _stopVideoPixThread();
	void _videoPixThreadedFunction();
	
	void _initParams();
	void _addToParams();
	
	void _addAppListeners();
	void _removeAppListeners();
	
	void _calculateDeltatime(); 
	
	ofParameterGroup params;
	ofParameter<bool> mBDrawPoints, mBDrawOutlines, mBDrawUsePosZ;
	ofParameter<float> mMaxDistToMatch;
	ofParameter<float> mMaxTimeToMatch;
	
	ofRectangle mSrcRect, mOutRect;
	
	std::string mGuiPrefix = "";
	
	bool mBHasAppListeners=false;
	
	uint64_t mCounterId = 0;
	
	py::module py_mediapipe;
	py::object py_landmarker;
	
	float mDrawPointSize = 2.f;

	//py::object py_ImageFormat;// = py_mediapipe.attr("ImageFormat");
	//py::object py_Image;// = py_mediapipe.attr("Image");

	bool mBSetup = false;
	bool mBHasNewData = false;
	
	float mLastTimeUpdateF = -1;
	float mDeltaTimeSmoothed = 0;
	
	std::mutex mMutex;
	std::atomic<bool> mHasNewThreadValues = false;
	
	std::mutex mMutexMediaPipe;
	std::atomic<bool> mBMediaPipeThreadFinished = true;
	
	std::atomic<bool> mBExiting = false;
	
	std::atomic<unsigned int> mThreadCallCount = 0;
	
	ofFpsCounter mFpsCounter;

//	static bool sBPyInterpreterInited;// = false;
	
	std::atomic<bool> mVideoThreadRunning;
	std::mutex mVideoPixMutex;
	std::thread mVideoThread;
	std::array<ofPixels, 2> mThreadVideoPixels;
	std::atomic<int> mVideoPixIndex;
	std::atomic<bool> mBNewVideoPixels;
	std::atomic<bool> mBVideoThreadDone;
	std::condition_variable mVideoThreadCondition;
	
	static int sNumPyInstances;
	
};
}

#endif
