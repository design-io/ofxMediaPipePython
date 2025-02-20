//
//  ofxMediaPipeRecorder.h
//  ShowMeFace
//
//  Created by Nick Hardeman on 5/17/24.
//

#pragma once
#include "ofxMediaPipeFrame.h"
#include <map>

namespace ofx::MediaPipe {
class Recorder {
public:
	// Based on ofxFFmpegRecorder
	using HighResClock = std::chrono::time_point<std::chrono::high_resolution_clock>;
	
	template<typename T>
	size_t addFrame( const std::vector<std::shared_ptr<T>>& aobjs ) {
		if( !mFrame ) {
			mFrame = std::make_shared<Frame>();
		}
		
		HighResClock now = std::chrono::high_resolution_clock::now();
		if( mNumAddedFrames == 0 ) {
			mRecordStartTime = now;
		}
		
		// https://en.cppreference.com/w/cpp/chrono/duration
		auto elapsed = now - mRecordStartTime;
		std::int64_t nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
		
		//	auto frame = make_shared<Frame>();
//		mJsonFile << mFrame->jsonify(nanos, aobjs );
		mFramesJson.push_back( mFrame->jsonify(nanos, aobjs ) );
		mDuration = nanos;
		
		mNumAddedFrames++;
		return mNumAddedFrames;
	}
	
	bool& isRecording() { return mBRecording; }
	bool startRecording( int awidth, int aheight, const of::filesystem::path& adirectory );
	bool startRecording( int awidth, int aheight, const of::filesystem::path& adirectory, std::string aFilename );
	void stopRecording();
	
	const std::int64_t& getDurationNanos();
	long long getDurationSeconds();
	std::size_t& getNumFrames();
	
	int getWidth() { return mWidth; }
	int getHeight() { return mHeight; }
	
	const of::filesystem::path & getLastRecordingPath(){
		return mLastPath; 
	}
	
protected:
	std::size_t mNumAddedFrames = 0;
	HighResClock mRecordStartTime;
	std::shared_ptr<ofx::MediaPipe::Frame> mFrame;
	bool mBRecording = false;
	of::filesystem::path mPath;
	std::int64_t mDuration = 0;
	int mWidth = 0;
	int mHeight = 0;
	of::filesystem::path mLastPath;
	
	ofJson mJson, mFramesJson;
	
};
}
