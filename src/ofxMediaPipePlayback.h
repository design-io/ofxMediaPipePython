//
//  ofxMediaPipePlayback.h
//  ShowMeFace
//
//  Created by Nick Hardeman on 5/17/24.
//

#pragma once
#include "ofxMediaPipeFrame.h"
#include "ofVideoBaseTypes.h"
#include <map>

namespace ofx::MediaPipe {
class Playback {
public:
	
	struct PlayFrame {
		bool bNewFrame = false;
		unsigned int currentFrameIndex = 0;
		unsigned int prevFrameIndex = 989823978;
		unsigned int totalFrames = 0;
	};
	
	// Based on ofxFFmpegRecorder
	using HighResClock = std::chrono::time_point<std::chrono::high_resolution_clock>;
	
	bool load(const of::filesystem::path& afile);
	bool isLoaded() { return mBLoaded; }
	of::filesystem::path& getFilepath() { return mFilepath; }
	
	void update();
	
	void play();
	void stop();
	bool isPaused();
	void setPaused( bool ab );
	void togglePaused();
	
	const std::int64_t& getPositionNanos();
	double getPositionSeconds();
	
	const std::int64_t& getDurationNanos();
	double getDurationSeconds();
	
	unsigned int getNumFrames();
	unsigned int getNumFrames(const TrackedObject::TrackedObjectType& atype);
	
	unsigned int getCurrentFrameIndex();
	unsigned int getCurrentFrameIndex(const TrackedObject::TrackedObjectType& atype);
	
	bool isFrameNew();
	bool isFrameNew( const TrackedObject::TrackedObjectType& atype );
	
	bool hasValidFrame(const TrackedObject::TrackedObjectType& atype);
	std::shared_ptr<Frame>& getFrame(const TrackedObject::TrackedObjectType& atype);
	
	bool isFaceFrameNew();
	std::vector< std::shared_ptr<Face> >& getFaces();
	
	bool isHandFrameNew();
	std::vector< std::shared_ptr<Hand> >& getHands();
	
	bool isPoseFrameNew();
	std::vector< std::shared_ptr<Pose> >& getPoses();
	
	void setOutRect(const ofRectangle& arect);// { mOutRect = arect; }
	
	bool isDone();
	void setLoopState(ofLoopType state);
	ofLoopType getLoopState();
	
	void setSpeed( float as ) {mSpeed=as;}
	float getSpeed() {return mSpeed;}
	
	int getWidth();
	int getHeight();
	
	void setSmoothingEnabled( bool ab ) { mBSmootingEnabled = ab;};
	void setSmoothing( float asmooth ) { mSmoothing = ofClamp( asmooth, 0.0f, 1.f); }
	
	// get all the frames for a type
	// helpful for iterating regardless of time
	std::vector< std::shared_ptr<Frame> > getAllFrames( const TrackedObject::TrackedObjectType& atype );
	
protected:
	void _checkPlayFrames();
	void _convertKeypointsToOutRect( std::vector< TrackedObject::Keypoint>& apts, const ofRectangle& arect );
	
	std::vector< std::shared_ptr<Frame> > mFrames;
	std::int64_t mPlayheadTime = 0;
	std::int64_t mDuration;
	HighResClock mLastUpdateTime;
	
	unsigned int mCurrentFrameIndex = 0;
	
	int mWidth = 0;
	int mHeight = 0;
	
	bool mBPaused = false;
	bool mBPlaying = false;
	bool mBDone = false;
	bool mBLoaded = false;
	
	ofRectangle mOutRect;
	
	ofLoopType mLoopType = OF_LOOP_NORMAL;
	
	std::map< TrackedObject::TrackedObjectType, PlayFrame > mPlayFrames;
	
	of::filesystem::path mFilepath;
	
	std::shared_ptr<Frame> dummyFrame;
	std::vector< std::shared_ptr<Face> > mFaces;
	std::vector< std::shared_ptr<Hand> > mHands;
	std::vector< std::shared_ptr<Pose> > mPoses;
	
	float mSmoothing = 0.6f;
	bool mBSmootingEnabled = false;
	
	float mSpeed = 1.f;
	
//	std::map< TrackedObject::TrackedObjectType, bool > mBNewFrames;
//	std::map< TrackedObject::TrackedObjectType, int > mLastUpdateFrames;
//	std::map< TrackedObject::TrackedObjectType, std::shared_ptr<Frame> > mCurrentFrames;
};
}
