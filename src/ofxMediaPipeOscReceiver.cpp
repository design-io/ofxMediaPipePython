//
//  ofxMediaPipeOscReceiver.cpp
//  TrackerMP
//
//  Created by Nick Hardeman on 10/14/24.
//

#include "ofxMediaPipeOscReceiver.h"
#include "ofGraphics.h"

using namespace ofx::MediaPipe;

//--------------------------------------------------------------
OscReceiver::OscReceiver() {
	mParams.setName("ofxMediaPipeOscReceiver");
	
	mBOscEnabled.set("Enabled", false);
	mPort.set("Port", 9009, 8000, 12000);

	mConnectionStatus.set("Status", "Disabled");
	
	mShiftX.set("ShiftX", 0.f, -1920.f, 1920.f*2.f);
	mShiftY.set("ShiftY", 0.f, -1080.f, 1080.f);
	mScaleX.set("ScaleX", 1.f, 0.5f, 2.f);
	mScaleY.set("ScaleY", 1.f, 0.5f, 2.f);
}

//--------------------------------------------------------------
OscReceiver::~OscReceiver() {
	_removeEventListeners();
}

//--------------------------------------------------------------
ofParameterGroup& OscReceiver::getParams() {

	if( mParams.size() < 1 ) {
#if defined(OF_ADDON_HAS_OFX_OSC)
//		mBOscEnabled.set("Enabled", true);
		mBOscEnabled = true;
		mParams.add(mBOscEnabled);
		mParams.add( mPort );
#endif
		mParams.add( mConnectionStatus );
		
		mParams.add(mShiftX);
		mParams.add(mShiftY);
		mParams.add(mScaleX);
		mParams.add(mScaleY);
	}
	
	_addAppEventListeners();
	return mParams;
}

//----------------------------------------------
float OscReceiver::getTimeSinceReceivedData() {
	return mTimeSinceReceivedData;
}

//----------------------------------------------
void OscReceiver::setup() {
	_addAppEventListeners();
}

//----------------------------------------------
void OscReceiver::close() {
	_removeEventListeners();
}

//----------------------------------------------
void OscReceiver::draw(float x, float y, float w, float h) const {
	
	auto drect = ofRectangle(x,y,w,h);
	
	float sx = w / mVideoRect.getWidth();
	float sy = h / mVideoRect.getHeight();
	
//	ofLogNotice("MediaPipe osc rx") << " video w: " << mVideoRect.getWidth() << " x " << mVideoRect.getHeight();
	
	
	{
		// faces
//		auto frect = ofRectangle( x, y, getWidth(TrackedObject::FACE), getHeight(TrackedObject::FACE));
//		frect.scaleTo(drect, OF_SCALEMODE_FIT);
//		auto frect = getRect(TrackedObject::FACE);
		ofPushMatrix(); {
			ofTranslate(x, y);
			ofScale( sx, sy );
			
//			ofNoFill();
//			auto frect = getRect(TrackedObject::FACE);
//			ofDrawRectangle(frect);
//			ofFill();
//			ofTranslate( frect.x, frect.y );
//			ofScale( frect.width, frect.height, 1.f );
			for( auto& face : mValidFaces ) {
//				face->getKeypointsNormalizedLineMesh().draw();
				face->getKeypointsLineMesh().draw();
			}
		} ofPopMatrix();
	}
	
	{
		// hands
//		auto frect = ofRectangle( x, y, getWidth(TrackedObject::HAND), getHeight(TrackedObject::HAND));
//		frect.scaleTo(drect, OF_SCALEMODE_FIT);
		ofPushMatrix(); {
			ofTranslate(x, y);
			ofScale( sx, sy );
//			ofTranslate( frect.x, frect.y );
//			ofScale( frect.width, frect.height, 1.f );
			for( auto& hand : mValidHands ) {
				hand->getKeypointsLineMesh().draw();
			}
		} ofPopMatrix();
	}
	
	{
		// poses
//		auto frect = ofRectangle( x, y, getWidth(TrackedObject::POSE), getHeight(TrackedObject::POSE));
//		frect.scaleTo(drect, OF_SCALEMODE_FIT);
		ofPushMatrix(); {
			ofTranslate(x, y);
			ofScale( sx, sy );
//			ofTranslate( frect.x, frect.y );
//			ofScale( frect.width, frect.height, 1.f );
			for( auto& pose : mValidPoses ) {
				pose->getKeypointsLineMesh().draw();
			}
		} ofPopMatrix();
	}
}

//----------------------------------------------
float OscReceiver::getWidth() const {
	return mVideoWidth;
}

//----------------------------------------------
float OscReceiver::getHeight() const {
	return mVideoHeight;
}

//----------------------------------------------
ofRectangle OscReceiver::getRect(const TrackedObject::TrackedObjectType& atype) {
	auto& tinfo = getTrackedObjectInfo(atype);
	if(tinfo.outRect.width < 1 ) {
		return mVideoRect;
	}
	return tinfo.outRect;
}

//----------------------------------------------
ofRectangle OscReceiver::getRect(const TrackedObject::TrackedObjectType& atype) const {
	if(mTInfoMap.count(atype) > 0) {
		return mTInfoMap.at(atype).outRect;
	}
	return mVideoRect;
}

////----------------------------------------------
//int OscReceiver::getWidth(const TrackedObject::TrackedObjectType& atype) {
//	if( mSizesMap.count(atype) > 0 ) {
//		return mSizesMap[atype].x;
//	}
//	return mVideoWidth;
//}
//
////----------------------------------------------
//int OscReceiver::getHeight(const TrackedObject::TrackedObjectType& atype) {
//	if( mSizesMap.count(atype) > 0 ) {
//		return mSizesMap[atype].y;
//	}
//	return mVideoHeight;
//}
//
////----------------------------------------------
//int OscReceiver::getWidth(const TrackedObject::TrackedObjectType& atype) const {
//	if( mSizesMap.count(atype) > 0 ) {
//		return mSizesMap.at(atype).x;
//	}
//	return mVideoWidth;
//}
//
////----------------------------------------------
//int OscReceiver::getHeight(const TrackedObject::TrackedObjectType& atype) const {
//	if( mSizesMap.count(atype) > 0 ) {
//		return mSizesMap.at(atype).y;
//	}
//	return mVideoHeight;
//}

//----------------------------------------------
int OscReceiver::getVideoWidth() {
	return mVideoWidth;
}

//----------------------------------------------
int OscReceiver::getVideoHeight() {
	return mVideoHeight;
}

//----------------------------------------------
bool OscReceiver::hasNewData( const TrackedObject::TrackedObjectType& atype ) {
//	if( mHasNewDataMap.count(atype) > 0 ) {
//		return mHasNewDataMap[atype];
//	}
	return getTrackedObjectInfo(atype).bHasNewData;
//	return false;
}

//----------------------------------------------
bool OscReceiver::hasNewFaceData() {
	return hasNewData( TrackedObject::FACE );
}

//----------------------------------------------
bool OscReceiver::hasNewHandData() {
	return hasNewData( TrackedObject::HAND );
}

//----------------------------------------------
bool OscReceiver::hasNewPoseData() {
	return hasNewData( TrackedObject::POSE );
}

//----------------------------------------------
std::vector< std::shared_ptr<ofx::MediaPipe::Face> >& OscReceiver::getFaces() {
	return mFaces;
}

//----------------------------------------------
std::vector< std::shared_ptr<ofx::MediaPipe::Hand> >& OscReceiver::getHands() {
	return mHands;
}

//----------------------------------------------
std::vector< std::shared_ptr<ofx::MediaPipe::Pose> >& OscReceiver::getPoses() {
	return mPoses;
}

//----------------------------------------------
std::vector< std::shared_ptr<ofx::MediaPipe::Face> >& OscReceiver::getValidFaces() {
	return mValidFaces;
}

//----------------------------------------------
std::vector< std::shared_ptr<ofx::MediaPipe::Hand> >& OscReceiver::getValidHands() {
	return mValidHands;
}

//----------------------------------------------
std::vector< std::shared_ptr<ofx::MediaPipe::Pose> >& OscReceiver::getValidPoses() {
	return mValidPoses;
}

//----------------------------------------------
std::shared_ptr<Face> OscReceiver::getFace( std::int32_t aid ) {
	unsigned int iid = (unsigned int)aid;
	for( auto& face : mFaces ) {
		if( face->ID == iid ) {
			return face;
		}
	}
	auto face = std::make_shared<Face>();
	face->ID = iid;
	mFaces.push_back(face);
	return face;
}

//----------------------------------------------
std::shared_ptr<Hand> OscReceiver::getHand( std::int32_t aid ) {
	unsigned int iid = (unsigned int)aid;
	for( auto& hand : mHands ) {
		if( hand->ID == iid ) {
			return hand;
		}
	}
	auto hand = std::make_shared<Hand>();
	hand->ID = iid;
	mHands.push_back(hand);
	return hand;
}

//----------------------------------------------
std::shared_ptr<Pose> OscReceiver::getPose( std::int32_t aid ) {
	unsigned int iid = (unsigned int)aid;
	for( auto& pose : mPoses ) {
		if( pose->ID == iid ) {
			return pose;
		}
	}
	auto pose = std::make_shared<Pose>();
	pose->ID = iid;
	mPoses.push_back(pose);
	return pose;
}

//--------------------------------------------------------------
void OscReceiver::update( ofEventArgs& args ) {
	float deltaTime = ofClamp(ofGetLastFrameTime(), 1.f / 2500.f, 1.f / 5.f );
	mTimeSinceReceivedData += deltaTime;
	
	_checkEnabled();
	
	for( auto& item : mTInfoMap ) {
		item.second.bHasNewData = false;
	}
	
	std::vector< std::shared_ptr<TrackedObject> > allObjs;
	if( mFaces.size() > 0 ) {
		allObjs.insert(allObjs.end(), mFaces.begin(), mFaces.end() );
	}
	if( mHands.size() > 0 ) {
		allObjs.insert(allObjs.end(), mHands.begin(), mHands.end() );
	}
	if( mPoses.size() > 0 ) {
		allObjs.insert(allObjs.end(), mPoses.begin(), mPoses.end() );
	}
	
	for( auto& obj : allObjs ) {
		obj->age += deltaTime;
		obj->trackingData.bFoundThisFrame = false;
		obj->trackingData.numFramesNotFound ++;
		if( obj->trackingData.numFramesNotFound > 40 ) {
			obj->bRemove = true;
		}
	}
	
	ofRemove( mFaces, TrackedObject::shouldRemove );
	ofRemove( mHands, TrackedObject::shouldRemove );
	ofRemove( mPoses, TrackedObject::shouldRemove );
#if defined(OF_ADDON_HAS_OFX_OSC)
	if( mOSCRX ) {
		
		if( !mFrame ) {
			mFrame = std::make_shared<Frame>();
		}
		
		unsigned int numMessages = 0;
		ofxOscMessage m;
		while(mOSCRX->getNextMessage(m)) {
			std::string address = m.getAddress();
			
			//ofLogNotice("OscReceiver::update") << "received data from " << address << " | " << ofGetFrameNum();
//			auto vec = ofSplitString( address, "/", true, true );
			// hm.setAddress("/ofxmp/frame");
//			m.setAddress("/ofxmp/faces");
//			m.setAddress("/ofxmp/hands");
//			m.setAddress("/ofxmp/poses");
			
//			send the width and height from the osc sender
			
			bool bWorldPos = false;
			std::optional<TrackedObject::TrackedObjectType> objType;
			if( address == "/ofxmp/frame/video" ) {
				if( m.getNumArgs() > 3 ) {
					mVideoWidth = m.getArgAsInt(2);
					mVideoHeight = m.getArgAsInt(3);
					mVideoRect.setWidth(mVideoWidth);
					mVideoRect.setHeight(mVideoHeight);
				}
			} else if( address == "/ofxmp/frame/faces" ) {
				if( m.getNumArgs() > 3 ) {
					auto& tinfo = getTrackedObjectInfo(TrackedObject::FACE);
					tinfo.outRect.x = m.getArgAsInt(0);
					tinfo.outRect.y = m.getArgAsInt(1);
					tinfo.outRect.width = m.getArgAsInt(2);
					tinfo.outRect.height = m.getArgAsInt(3);
					
				}
			} else if( address == "/ofxmp/frame/hands" ) {
				if( m.getNumArgs() > 3 ) {
					auto& tinfo = getTrackedObjectInfo(TrackedObject::HAND);
					tinfo.outRect.x = m.getArgAsInt(0);
					tinfo.outRect.y = m.getArgAsInt(1);
					tinfo.outRect.width = m.getArgAsInt(2);
					tinfo.outRect.height = m.getArgAsInt(3);
				}
			} else if( address == "/ofxmp/frame/poses" ) {
				if( m.getNumArgs() > 3 ) {
					auto& tinfo = getTrackedObjectInfo(TrackedObject::POSE);
					tinfo.outRect.x = m.getArgAsInt(0);
					tinfo.outRect.y = m.getArgAsInt(1);
					tinfo.outRect.width = m.getArgAsInt(2);
					tinfo.outRect.height = m.getArgAsInt(3);
				}
			} else if( address == "/ofxmp/faces" ) {
				objType = TrackedObject::FACE;
			} else if( address == "/ofxmp/hands" ) {
				objType = TrackedObject::HAND;
			} else if( address == "/ofxmp/poses" ) {
				objType = TrackedObject::POSE;
			} else if( address == "/ofxmp/facesW" ) {
				objType = TrackedObject::FACE;
				bWorldPos = true;
			} else if(address == "/ofxmp/handsW") {
				objType = TrackedObject::HAND;
				bWorldPos = true;
			} else if(address == "/ofxmp/posesW") {
				objType = TrackedObject::POSE;
				bWorldPos = true;
			}
			
			if( objType.has_value() && m.getNumArgs() > 2) {
				
				getTrackedObjectInfo(objType.value()).bHasNewData = true;
				
				std::shared_ptr<TrackedObject> tobj;
				std::int64_t frame = m.getArgAsInt64(0);
				std::int32_t tid = m.getArgAsInt32(1);
				
				if( objType.value() == TrackedObject::FACE ) {
					tobj = getFace(tid);
				} else if( objType.value() == TrackedObject::HAND ) {
					tobj = getHand(tid);
				} else if( objType.value() == TrackedObject::POSE ) {
					tobj = getPose(tid);
				}
				
				if( tobj ) {
					
					//ofLogNotice("MpOscReceiver") << "id: " << tid << " obj->id: " << tobj->ID << " age: " << tobj->age << " world: " << bWorldPos << " frame: " << frame << " obj frame: " << tobj->trackingData.mostRecentFrame << " | " << ofGetFrameNum();
					
					tobj->trackingData.bFoundThisFrame = true;
					tobj->trackingData.numFramesNotFound = 0;
					if( tobj->trackingData.mostRecentFrame < frame ) {
						tobj->trackingData.mostRecentFrame = frame;
						mFrame->setup(m, tobj, bWorldPos );
						// now lets set the pos by multiplying the posN
//						float tw = (float)getWidth(tobj->getType());
//						float th = (float)getHeight(tobj->getType());
						ofRectangle trect = getRect(tobj->getType());
						for( auto& kp : tobj->keypoints ) {
							
							if( bWorldPos ) {
								
							} else {
								kp.posN *= glm::vec3( mScaleX, mScaleY, mScaleX );
							}
							
							kp.pos.x = kp.posN.x * trect.width + trect.x + mShiftX;
							kp.pos.y = kp.posN.y * trect.height + trect.y + mShiftY;
							kp.pos.z = kp.posN.z * trect.width;
						}
						tobj->updateFromKeypoints();
					}
					
					if( bWorldPos ) {
						tobj->trackingData.worldPositionsSet = true;
					} else {
						tobj->trackingData.positionsSet = true;
						tobj->trackingData.fps.newFrame();
					}
				}
			}
			
			mTimeSinceReceivedData = 0.f;
			
			numMessages++;
			if( numMessages > 999 ) {
//				mOSCRX->clea
				break;
			}
		}
	} else {
		mTimeSinceReceivedData = 99.f;
	}
#else
	mTimeSinceReceivedData = 99.f;
#endif
	
	mValidFaces.clear();
	for( auto& face : mFaces ) {
		if( face->trackingData.positionsSet && face->trackingData.worldPositionsSet ) {
			mValidFaces.push_back( face );
		}
	}
	
	mValidHands.clear();
	for( auto& hand : mHands ) {
		if( hand->trackingData.positionsSet && hand->trackingData.worldPositionsSet ) {
			mValidHands.push_back( hand );
		}
	}
	
	mValidPoses.clear();
	for( auto& pose : mPoses ) {
		if( pose->trackingData.positionsSet && pose->trackingData.worldPositionsSet ) {
			mValidPoses.push_back( pose );
		}
	}
	
}

//--------------------------------------------------------------
bool OscReceiver::isSetupForRx() {
#if defined(OF_ADDON_HAS_OFX_OSC)
	if (mOSCRX) {
		return true;
	}
#endif
	return false;
}

//---------------------------------------------
void OscReceiver::deleteReceiver() {
#if defined(OF_ADDON_HAS_OFX_OSC)
	if(mOSCRX) {
		mOSCRX->stop();
		mOSCRX.reset();
	}
#endif
}

//----------------------------------------------
void OscReceiver::setupForRecieve(int port) {
#if defined(OF_ADDON_HAS_OFX_OSC)
	mOSCRX = std::make_shared<ofxOscReceiver>();
	ofxOscReceiverSettings settings;
	settings.port = port;
	settings.reuse = true;
	settings.start = true;
	mOSCRX->setup(settings);
#endif
}

//--------------------------------------------------------------
void OscReceiver::onPortParamChanged(int& aPort) {
	if (isSetupForRx()) {
		deleteReceiver();
	}
	
	mNextCheckTimef = ofGetElapsedTimef() + 2.f;
}

//--------------------------------------------------------------
OscReceiver::TrackedObjectRxInfo& OscReceiver::getTrackedObjectInfo(TrackedObject::TrackedObjectType atype) {
	if( mTInfoMap.count(atype) < 1 ) {
		mTInfoMap[atype] = TrackedObjectRxInfo();
		mTInfoMap[atype].bHasNewData = false; // just in case
		mTInfoMap[atype].outRect = ofRectangle(0,0,0,0);
	}
	return mTInfoMap[atype];
}

//---------------------------------------------
void OscReceiver::_checkEnabled() {
	if (mBOscEnabled) {
		float etimef = ofGetElapsedTimef();
		
		if (etimef > mNextCheckTimef) {
			
			if (!isSetupForRx()) {
				setupForRecieve(mPort);
				ofLogNotice("ofx::MediaPipe::OscReceiver") << " setting up  Receiver: port " << mPort << std::endl;
			}
			
			mNextCheckTimef = etimef + 1.f;
		}
		//dio::GlobalParams::SharedNetMan.update();
		if (getTimeSinceReceivedData() < 10) {
			mConnectionStatus = std::string("Connected");
		} else {
			mConnectionStatus = std::string("Not Connected");
		}
	} else {
		if (isSetupForRx()) {
			deleteReceiver();
		}
		mConnectionStatus = std::string("Not Connected");
	}
}

//---------------------------------------------
void OscReceiver::_addAppEventListeners() {
	if (!mBHasAppListeners) {
		ofAddListener(ofEvents().update, this, &OscReceiver::update, OF_EVENT_ORDER_BEFORE_APP);
//		mBroadcastIp.addListener(this, &NetworkManager::onNetManIpParamChanged);
		mPort.addListener(this, &OscReceiver::onPortParamChanged);
//		mEnabled.addListener(this, &NetworkManager::onNetManEnabledParamChanged);
//		mBIsServer.addListener(this, &NetworkManager::onNetManIsServerParamChanged);
	}
	mBHasAppListeners = true;
}

//---------------------------------------------
void OscReceiver::_removeEventListeners() {
	if (mBHasAppListeners) {
		ofRemoveListener(ofEvents().update, this, &OscReceiver::update, OF_EVENT_ORDER_BEFORE_APP);
//		mBroadcastIp.removeListener(this, &NetworkManager::onNetManIpParamChanged);
		mPort.removeListener(this, &OscReceiver::onPortParamChanged);
//		mEnabled.removeListener(this, &NetworkManager::onNetManEnabledParamChanged);
//		mBIsServer.removeListener(this, &NetworkManager::onNetManIsServerParamChanged);
	}
	mBHasAppListeners = false;
}



