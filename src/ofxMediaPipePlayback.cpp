//
//  ofxMediaPipePlayback.cpp
//  ShowMeFace
//
//  Created by Nick Hardeman on 5/17/24.
//

#include "ofxMediaPipePlayback.h"

using std::shared_ptr;
using std::make_shared;
using std::vector;
using std::string;

using namespace ofx::MediaPipe;

//--------------------------------------------------------------
bool Playback::load(const of::filesystem::path& afile) {
	mFrames.clear();
	mCurrentFrameIndex = 0;
	mPlayheadTime = 0;
	mPlayFrames.clear();
	mDuration = 0;
	mOutRect.set(0,0,0,0);
	mBDone = false;
	mBLoaded = false;
	
	if( afile.empty() ) {
		return false;
	}
	
	mFilepath.clear();
	
//	ofFile jsonFile(afile);
	
	ofJson tjson = ofLoadJson(afile);
	if( !tjson.is_null() ) {
		
		if( tjson.count("frames") < 1 ) {
			ofLogError("Playback::load") << "could not load file from: " << afile;
			return false;
		}
		
		mWidth = tjson["width"];
		mHeight = tjson["height"];
		
		ofLogNotice("Playback::load") << "loaded from " << afile << " " << mWidth << " x " << mHeight;
		
		auto jframes = tjson["frames"];
		for( auto& jframe : jframes ) {
			if( !jframe.empty() ) {
				// we have a frame here
				auto frame = make_shared<Frame>();
				if(frame->setup(jframe)) {
					mFrames.push_back(frame);
				}
			}
		}
		
		_checkPlayFrames();
		
		mDuration = 0;
		// count up the total frames //
		int ii = 0;
		for( auto& frame : mFrames ) {
			mPlayFrames[frame->getType()].totalFrames++;
//			ofLogNotice("Frame: ")<< ii << " - " << frame->getTimestamp();
			if( frame->getTimestamp() > mDuration ) {
				mDuration = frame->getTimestamp();
			}
			ii++;
		}
		
		mOutRect.set( 0,0,0,0);
		
		std::cout << afile << " setting out rect to " << ofRectangle(0,0,mWidth, mHeight) << std::endl; 
		setOutRect( ofRectangle(0,0,mWidth, mHeight));
		
		play();
		mBLoaded = true;
		mFilepath = afile;
		return true;
	}
	
	ofLogError("Playback::load") << "error loading from " << afile;
	return false;
}


//--------------------------------------------------------------
void Playback::update() {
	
	_checkPlayFrames();
	
	for( auto& iter : mPlayFrames ) {
		iter.second.bNewFrame = false;
	}
	
	if( mFrames.size() < 1 ) {
		mBDone = true;
		return;
	}
	
	mBDone = false;
	
	auto now = std::chrono::high_resolution_clock::now();
	auto deltaD = now - mLastUpdateTime;
	mLastUpdateTime = now;
	
	if( !mBPlaying || mBPaused ) {
		return;
	}
	
	std::int64_t deltaNanos = std::chrono::duration_cast<std::chrono::nanoseconds>(deltaD).count();
	mPlayheadTime += ((double)deltaNanos * (double)mSpeed);
	
	
	auto playheadSecs = getPositionSeconds();
	
//	ofLogNotice("Playback::update : playhead: ") << mPlayheadTime << " / " << mFrames[mCurrentFrameIndex]->getTimestamp() << " frame: " << mCurrentFrameIndex << " / " << mFrames.size();
	
	size_t numFrames = mFrames.size();
	if( mCurrentFrameIndex < numFrames ) {
		for( auto& iter : mPlayFrames ) {
			iter.second.prevFrameIndex = iter.second.currentFrameIndex;
		}
		
		// loop through the different types to get updated frames 
		while( mCurrentFrameIndex < numFrames ) {
			mPlayFrames[mFrames[mCurrentFrameIndex]->getType()].currentFrameIndex = mCurrentFrameIndex;
			
//			ofLogNotice("Checking frame index ") << mCurrentFrameIndex << " num frames: " << numFrames << " fts: " <<  mFrames[mCurrentFrameIndex]->getTimestamp() << " / " << mPlayheadTime;
//			auto tsSeconds = std::chrono::duration_cast<std::chrono::seconds>( std::chrono::nanoseconds(mFrames[mCurrentFrameIndex]->getTimestamp()) ).count();
//			ofLogNotice("Checking frame index ") << mCurrentFrameIndex << " num frames: " << numFrames << " fts: " <<  tsSeconds << " / " << playheadSecs;
			
			if( mFrames[mCurrentFrameIndex]->getTimestamp() > mPlayheadTime ) {
				break;
			}
			mCurrentFrameIndex++;
		}
		
		for( auto& iter : mPlayFrames ) {
			if(iter.second.prevFrameIndex != iter.second.currentFrameIndex) {
				iter.second.bNewFrame = true;
			}
		}
	}
	
	if( mCurrentFrameIndex >= numFrames ) {
		mBDone = true;
		mPlayheadTime = mDuration;
		
		if( mLoopType == OF_LOOP_NORMAL ) {
			mCurrentFrameIndex = 0;
			for( auto& iter : mPlayFrames ) {
				iter.second.bNewFrame = true;
			}
			mPlayheadTime = 0;
		}
	}
	
	if( isFrameNew(TrackedObject::FACE) && hasValidFrame(TrackedObject::FACE) ) {
		if( auto frame = getFrame(TrackedObject::FACE) ) {
			for( auto& obj : mFaces ) {
				obj->bRemove = true;
			}
			for( auto& fobj : frame->getFaces() ) {
				bool bHasIt = false;
				for( auto& obj : mFaces ) {
					if( obj->ID == fobj->ID ) {
						obj->updateFrom(fobj);
						obj->bRemove = false;
						bHasIt = true;
						break;
					}
				}
				if( !bHasIt ) {
					auto nface = std::make_shared<Face>( *(fobj.get()) );
					mFaces.push_back(nface);
				}
			}
		}
	}
	
	if( isFrameNew(TrackedObject::HAND) && hasValidFrame(TrackedObject::HAND) ) {
		if( auto frame = getFrame(TrackedObject::HAND) ) {
			for( auto& obj : mHands ) {
				obj->bRemove = true;
			}
			for( auto& fobj : frame->getHands() ) {
				bool bHasIt = false;
				for( auto& obj : mHands ) {
					if( obj->ID == fobj->ID ) {
						obj->updateFrom(fobj);
						obj->bRemove = false;
						bHasIt = true;
						break;
					}
				}
				if( !bHasIt ) {
					auto nhand = std::make_shared<Hand>( *(fobj.get()) );
					mHands.push_back(nhand);
				}
			}
		}
	}
	
	if( isFrameNew(TrackedObject::POSE) && hasValidFrame(TrackedObject::POSE) ) {
		if( auto frame = getFrame(TrackedObject::POSE) ) {
			for( auto& obj : mPoses ) {
				obj->bRemove = true;
			}
//			mPoses = frame->getPoses();
			
			for( auto& fobj : frame->getPoses() ) {
				bool bHasIt = false;
				for( auto& obj : mPoses ) {
					if( obj->ID == fobj->ID ) {
						if(mBSmootingEnabled) {
							obj->updateFromPoseWithSmoothing(fobj, mSmoothing );
						} else {
							obj->updateFrom(fobj);
						}
						obj->bRemove = false;
						bHasIt = true;
						break;
					}
				}
				if( !bHasIt ) {
					auto npose = std::make_shared<Pose>( *(fobj.get()) );
//					std::cout << " adding poses frame " << std::endl;
					mPoses.push_back(npose);
				}
			}
		}
	}
	
	ofRemove( mFaces, TrackedObject::shouldRemove );
	ofRemove( mHands, TrackedObject::shouldRemove );
	ofRemove( mPoses, TrackedObject::shouldRemove );
	
}

//--------------------------------------------------------------
void Playback::play() {
	if( !mBPlaying ) {
		mBPaused = false;
		mCurrentFrameIndex = 0;
		mPlayheadTime = 0;
		mLastUpdateTime = std::chrono::high_resolution_clock::now();
	}
	
	mBPlaying = true;
}

//--------------------------------------------------------------
void Playback::stop() {
	mCurrentFrameIndex = 0;
	mPlayheadTime = 0;
	mBPlaying = false;
	mBPaused = false;
}

//--------------------------------------------------------------
bool Playback::isPaused() {
	return mBPaused;
}

//--------------------------------------------------------------
void Playback::setPaused( bool ab ) {
	mBPaused = ab;
}

//--------------------------------------------------------------
void Playback::togglePaused() {
	mBPaused = !mBPaused;
}

//--------------------------------------------------------------
const std::int64_t& Playback::getPositionNanos() {
	return mPlayheadTime;
}

//--------------------------------------------------------------
double Playback::getPositionSeconds() {
//	return std::chrono::duration_cast<std::chrono::seconds>( std::chrono::nanoseconds(mPlayheadTime) ).count();
	std::chrono::nanoseconds ns_duration(mPlayheadTime);
	std::chrono::duration<double> seconds_duration = ns_duration;
	return seconds_duration.count();
}

//--------------------------------------------------------------
const std::int64_t& Playback::getDurationNanos() {
	return mDuration;
}

//--------------------------------------------------------------
double Playback::getDurationSeconds() {
//	return std::chrono::duration_cast<std::chrono::seconds>( std::chrono::nanoseconds(mDuration) ).count();
	std::chrono::nanoseconds ns_duration(mPlayheadTime);
	std::chrono::duration<double> seconds_duration = ns_duration;
	return seconds_duration.count();
}

//--------------------------------------------------------------
unsigned int Playback::getNumFrames() {
	return mFrames.size();
}

//--------------------------------------------------------------
unsigned int Playback::getNumFrames(const TrackedObject::TrackedObjectType& atype) {
	_checkPlayFrames();
	return mPlayFrames[atype].totalFrames;
}

//--------------------------------------------------------------
unsigned int Playback::getCurrentFrameIndex() {
	return mCurrentFrameIndex;
}

//--------------------------------------------------------------
unsigned int Playback::getCurrentFrameIndex(const TrackedObject::TrackedObjectType& atype) {
	_checkPlayFrames();
	return mPlayFrames[atype].currentFrameIndex;
}

//--------------------------------------------------------------
bool Playback::isFrameNew() {
	for( auto& iter : mPlayFrames ) {
		if(iter.second.bNewFrame ) {return true;}
	}
	return false;
}

//--------------------------------------------------------------
bool Playback::isFrameNew( const TrackedObject::TrackedObjectType& atype ) {
	if( !hasValidFrame(atype) ) return false;
	return mPlayFrames[atype].bNewFrame;
}

//--------------------------------------------------------------
bool Playback::hasValidFrame(const TrackedObject::TrackedObjectType& atype) {
	if( mFrames.size() < 1 ) return false;
	_checkPlayFrames();
	if( mPlayFrames[atype].totalFrames < 1 ) return false;
	if( mPlayFrames[atype].currentFrameIndex < mFrames.size() ) {
		return mFrames[mPlayFrames[atype].currentFrameIndex ] ? true : false;
	}
	return false;
//	if( mCurrentFrameIndex >= mFrames.size() ) return false;
//	return (mFrames[mCurrentFrameIndex]) ? true : false;
}

//--------------------------------------------------------------
std::shared_ptr<Frame>& Playback::getFrame(const TrackedObject::TrackedObjectType& atype) {
	if( !hasValidFrame(atype) ) {
		return dummyFrame;
	}
	if(mFrames.size() == 1) return mFrames[0];
	return mFrames[mPlayFrames[atype].currentFrameIndex];
}

//--------------------------------------------------------------
bool Playback::isFaceFrameNew() {
	return isFrameNew( TrackedObject::FACE );
}

//--------------------------------------------------------------
std::vector< std::shared_ptr<Face> >& Playback::getFaces() {
	return mFaces;
}

//--------------------------------------------------------------
bool Playback::isHandFrameNew() {
	return isFrameNew( TrackedObject::HAND );
}

//--------------------------------------------------------------
std::vector< std::shared_ptr<Hand> >& Playback::getHands() {
	return mHands;
}

//--------------------------------------------------------------
bool Playback::isPoseFrameNew() {
	return isFrameNew( TrackedObject::POSE );
}

//--------------------------------------------------------------
std::vector< std::shared_ptr<Pose> >& Playback::getPoses() {
	return mPoses;
}

//--------------------------------------------------------------
void Playback::setOutRect(const ofRectangle &arect) {
	if( mOutRect != arect ) {
		for( auto& frame : mFrames ) {
			
			for( auto& face : frame->getFaces() ) {
				_convertKeypointsToOutRect(face->keypoints,arect);
			}
			for( auto& hand : frame->getHands() ) {
				_convertKeypointsToOutRect(hand->keypoints,arect);
			}
			for( auto& pose : frame->getPoses() ) {
				_convertKeypointsToOutRect(pose->keypoints,arect);
			}
		}
	}
	mOutRect = arect;
}

//--------------------------------------------------------------
bool Playback::isDone() {
	return mBDone;
}

//--------------------------------------------------------------
void Playback::setLoopState(ofLoopType state) {
	mLoopType = state;
}

//--------------------------------------------------------------
ofLoopType Playback::getLoopState() {
	return mLoopType;
}

//--------------------------------------------------------------
int Playback::getWidth() {
	return mWidth;
}

//--------------------------------------------------------------
int Playback::getHeight() {
	return mHeight;
}

//--------------------------------------------------------------
std::vector< std::shared_ptr<Frame> > Playback::getAllFrames( const TrackedObject::TrackedObjectType& atype ) {
	std::vector< std::shared_ptr<Frame> > rframes;
	for( auto& frame : mFrames ) {
		if(frame->getType() == atype) {
			rframes.push_back(frame);
		}
	}
	return rframes;
}

//--------------------------------------------------------------
void Playback::_checkPlayFrames() {
	if(mPlayFrames.size() < 1 ) {
		mPlayFrames[TrackedObject::FACE] = PlayFrame();
		mPlayFrames[TrackedObject::HAND] = PlayFrame();
		mPlayFrames[TrackedObject::POSE] = PlayFrame();
	}
}

//--------------------------------------------------------------
void Playback::_convertKeypointsToOutRect( std::vector<TrackedObject::Keypoint>& apts, const ofRectangle& arect ) {
	for( auto& kp : apts ) {
		kp.pos = kp.posN;
		kp.pos.x *= arect.width;
		kp.pos.y *= arect.height;
		kp.pos.x += arect.x;
		kp.pos.y += arect.y;
		
		kp.pos.z *= arect.width;
	}
}

////--------------------------------------------------------------
//void Playback::_updateFrameObjects(TrackedObject::TrackedObjectType atype) {
//	if( isFrameNew(atype) && hasValidFrame(atype) ) {
//		if( auto frame = getFrame(atype) ) {
//			std::vector< std::shared_ptr<TrackedObject> > frameObjects;
//			std::vector< std::shared_ptr<TrackedObject> > trackedObjects;// = mFaces;
//			
//			if( atype == TrackedObject::FACE ) {
//				frameObjects.insert(frameObjects.end(), frame->getFaces().begin(), frame->getFaces().end() );
//				trackedObjects.insert(trackedObjects.end(), mFaces.begin(), mFaces.end() );
//			} else if( atype == TrackedObject::HAND ) {
//				frameObjects.insert(frameObjects.end(), frame->getHands().begin(), frame->getHands().end() );
//				trackedObjects.insert(trackedObjects.end(), mHands.begin(), mHands.end() );
//			} else if( atype == TrackedObject::POSE ) {
//				frameObjects.insert(frameObjects.end(), frame->getPoses().begin(), frame->getPoses().end() );
//				trackedObjects.insert(trackedObjects.end(), mPoses.begin(), mPoses.end() );
//			}
//			
//			for( auto& obj : trackedObjects ) {
//				obj->bRemove = true;
//			}
//			for( auto& fobj : frameObjects ) {
//				bool bHasIt = false;
//				for( auto& obj : trackedObjects ) {
//					if( obj->ID == fobj->ID ) {
//						obj->bRemove = false;
//						bHasIt = true;
//						break;
//					}
//				}
//				if( !bHasIt ) {
//					if( atype == TrackedObject::FACE ) {
//						auto nface = std::make_shared<Face>( *(fobj.get()) );
//						mFaces.push_back(nface);
//					} else if( atype == TrackedObject::HAND ) {
//						auto nface = std::make_shared<Hand>( *(fobj.get()) );
//						mHands.push_back(nface);
//					} else if( atype == TrackedObject::POSE ) {
//						auto nface = std::make_shared<Pose>( *(fobj.get()) );
//						mPoses.push_back(nface);
//					}
//					
//				}
//			}
//		}
//	}
//}




