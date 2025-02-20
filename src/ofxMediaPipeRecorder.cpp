//
//  ofxMediaPipeRecorder.cpp
//  ShowMeFace
//
//  Created by Nick Hardeman on 5/17/24.
//

#include "ofxMediaPipeRecorder.h"

using namespace ofx::MediaPipe;
using namespace std;

//---------------------------------
bool Recorder::startRecording(int awidth, int aheight, const of::filesystem::path& adirectory) {
	std::string filename = ofGetTimestampString()+".json";
	return startRecording(awidth, aheight, adirectory, filename);
}

//---------------------------------
bool Recorder::startRecording(int awidth, int aheight, const of::filesystem::path& adirectory, std::string aFilename ) {
	if( mBRecording ) {
		ofLogWarning("Recorder::startRecording") << "recording already in progress!";
		return false;
	}
	
	if( !ofDirectory::doesDirectoryExist(adirectory) ) {
		// create directory
		ofDirectory::createDirectory(adirectory, true, true);
	}
	
	mPath = ofFilePath::addTrailingSlash(adirectory)+aFilename;
	if( mPath.extension() != ".json") {
		ofLogWarning("Recorder::startRecording") << "extenstion must be json, not" << mPath.extension();
		return false;
	}
	
	mLastPath = mPath;
	
//	mJsonFile.clear();
//	mJsonFile.close();
////	bool open(const of::filesystem::path & path, Mode mode = ReadOnly, bool binary = true);
//	if(!mJsonFile.open(mPath, ofFile::WriteOnly, true)) {
//		ofLogWarning("Recorder::startRecording") << "error opening file to save to " << mPath;
//		return false;
//	}
	
	mWidth = awidth;
	mHeight = aheight;
	
//	ofJson jvideo;
//	jvideo["width"] = awidth;
//	jvideo["height"] = awidth;
//	
//	ofJson jheader;
//	jheader["video"] = jvideo;
//	mJsonFile << jheader;
	
	mJson.clear();
	
	mJson["width"] = mWidth;
	mJson["height"] = mHeight;
	
	mFramesJson.clear();
	
	mNumAddedFrames = 0;
	mDuration = 0;
	
	mBRecording=true;
	return true;
}

//---------------------------------
void Recorder::stopRecording() {
	if(mBRecording) {
		if(mNumAddedFrames > 0 && !mPath.empty()) {
			mJson["frames"] = mFramesJson;
			ofSaveJson(mPath, mJson);
		}
//		if(mJsonFile.is_open() ) {
//			mJsonFile.close();
//		}
	}
	mBRecording = false;
}

//---------------------------------
const std::int64_t& Recorder::getDurationNanos() {
	return mDuration;
}

//---------------------------------
long long Recorder::getDurationSeconds() {
	return std::chrono::duration_cast<std::chrono::seconds>( std::chrono::nanoseconds(mDuration) ).count();
}

//---------------------------------
std::size_t& Recorder::getNumFrames() {
	return mNumAddedFrames;
}


