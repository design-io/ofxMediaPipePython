//
//  ofxMediaPipeHand.cpp
//  HandShapeController
//
//  Created by Nick Hardeman on 4/25/24.
//

#include "ofxMediaPipeHand.h"
#include "ofGraphics.h"
#include "ofxMediaPipeUtils.h"

using namespace ofx::MediaPipe;

using std::string;
using std::shared_ptr;
using std::vector;
using std::make_shared;
using std::cout;
using std::endl;

glm::vec3 Hand::Finger::defaultVec3 = {0.f, 0.f, 0.f};


//-------------------------------------------
glm::vec3& Hand::Finger::getTip() {
	if( keypoints.size() ) {
		return keypoints.back().pos;
	}
	return defaultVec3;
}

//-------------------------------------------
glm::vec3& Hand::Finger::getTipWorld() {
	if( keypoints.size() ) {
		return keypoints.back().posWorld;
	}
	return defaultVec3;
}

//-------------------------------------------
void Hand::Finger::draw(bool aBUseZ) {
	glm::vec3 tv = {0.f, 0.f, 0.f};
	for( int i = 0; i < keypoints.size(); i++ ) {
		auto& pt = keypoints[i];
		tv = pt.pos;
		if( !aBUseZ ) {
			tv.z = 0.f;
		}
		ofSetColor( color );
		ofDrawCircle(tv, 2);
	}
}

//-------------------------------------------
void Hand::Finger::drawInfo( bool aBUseZ ) {
	std::string openStr = (isOpen(mMaxDegForOpenFinger) ? "open" : "closed");
	openStr += "\nto: "+ofToString(timeOpen, 1);
	auto tip = getTip();
	if( !aBUseZ ) {
		tip.z = 0.;
	}
	ofDrawBitmapStringHighlight("c: "+openStr, tip + glm::vec3(20.f, 0.0f,0.f));
}

//-------------------------------------------
void Hand::Finger::drawWorld() {
	glm::vec3 tv = {0.f, 0.f, 0.f};
	for( auto& pt : keypoints ) {
		tv = pt.posWorld;
		ofDrawCircle(tv, 4);
	}
}

//-------------------------------------------
std::string Hand::sGetFingerTypeAsString(const FingerType& aftype) {
	if( aftype == FINGER_THUMB ) {
		return "Thumb";
	} else if( aftype == FINGER_INDEX ) {
		return "Index";
	} else if( aftype == FINGER_MIDDLE ) {
		return "Middle";
	} else if( aftype == FINGER_RING ) {
		return "Ring";
	} else if( aftype == FINGER_PINKY ) {
		return "Pinky";
	}
	return "Unknown";
}

//-------------------------------------------
void Hand::setupFingers() {
	fingers.clear();
	
	vector<ofColor> fcolors = {
		ofColor::cyan,
		ofColor::magenta,
		ofColor::yellow,
		ofColor::blue,
		ofColor::black
	};
	
	for( int k = 0; k < FINGER_TOTAL; k++ ) {
		auto fing = make_shared<Finger>();
		fing->type = (FingerType)k;
		fing->name = sGetFingerTypeAsString(fing->type);
		
		fing->color = fcolors[k % (int)fcolors.size()];
		fing->keypoints.assign(4, TrackedObject::Keypoint() );
		fing->handID = ID;
		fingers.push_back(fing);
	}
}

//-------------------------------------------
void Hand::updateFingers() {
	if( fingers.size() < 1 ) {
		setupFingers();
	}
	
	for( auto& fing : fingers ) {
		int k = (int)fing->type;
		int startIndex = THUMB_CMC+k*4;
		int findex = 0;
		for( int fi = startIndex; fi < startIndex+4; fi++ ) {
			fing->keypoints[findex] = keypoints[fi];
			findex++;
		}
		
		fing->mMaxDegForOpenFinger = mMaxDegForOpenFinger;
		
		fing->closedAmountRad = 0.f;
		if( fing->keypoints.size() >= 4 ) {
			fing->anglesRad.assign(3, 0.0f);
			
			// lets figure out some angles //
			auto v1 = glm::vec3(0.f, -1.f, 0.0f );
			auto v2 = Utils::normalize(fing->keypoints[1].posWorld-fing->keypoints[0].posWorld, glm::vec3(1.f, 0.0, 0.0f));
			fing->anglesRad[0] = glm::angle(v1, v2);
			
			v1 = v2;
			v2 = Utils::normalize(fing->keypoints[2].posWorld-fing->keypoints[1].posWorld, glm::vec3(1.f, 0.0, 0.0f));
			fing->anglesRad[1] = glm::angle(v1, v2 );
			
			v1 = v2;
			v2 = Utils::normalize(fing->keypoints[3].posWorld-fing->keypoints[2].posWorld, glm::vec3(1.f, 0.0, 0.0f));
			fing->anglesRad[2] = glm::angle(v1, v2 );
			
			fing->closedAmountRad = fabsf(fing->anglesRad[1]) + fabsf(fing->anglesRad[2]);
		}
		
		fing->handPos = getPosition();
	}
}

//-------------------------------------------
void Hand::updateFrom( std::shared_ptr<Hand>& aother ) {
	keypoints = aother->keypoints;
	handed = aother->handed;
	index = aother->index;
	
	updateFromKeypoints();
	updateFingers();
}

//-------------------------------------------
void Hand::updateFromHandWithSmoothing( std::shared_ptr<Hand> aother, float pct ) {
	handed = aother->handed;
	index = aother->index;
	updateKeypointsFromOtherWithSmoothing(aother, pct);
	updateFingers();
}

//-------------------------------------------
void Hand::updateFromKeypoints() {
//	if(keypoints.size() > 1 ) {
//		pos = keypoints[0].pos;
//		posN = keypoints[0].posN;
//		posWorld = keypoints[0].posWorld;
//	}
	
	updateDrawMeshes();
	
	if( mKeypointsLineMesh.getNumIndices() < 1 && keypoints.size() > 1 ) {
		mKeypointsLineMesh.addIndices({
			0, 1,
			1, 2,
			2, 3,
			3, 4,
			0, 5,
			5, 6,
			6, 7,
			7, 8,
			5, 9,
			9, 10,
			10, 11,
			11, 12,
			9, 13,
			13, 14,
			14, 15,
			15, 16,
			13, 17,
			17, 18,
			18, 19,
			19, 20,
			0, 17
		});
	}
	
	
	//		https://developers.google.com/mediapipe/solutions/vision/hand_landmarker/python#video_2
	//		The 21 hand landmarks are also presented in world coordinates. Each landmark is composed of x, y, and z, representing real-world 3D coordinates in meters with the origin at the hand’s geometric center.
	for( size_t j = 1; j < keypoints.size(); j++ ) {
		keypoints[j].posWorld += getPositionWorld();
	}
	
	if( keypoints.size() > 17 ) {
		// calculate palm things
		auto& posWrist = keypoints[0].pos;
		auto& posPalmL = keypoints[17].pos;
		auto& posPalmR = keypoints[5].pos;
		
		glm::vec3 u = (posPalmL-posPalmR); //toGlm(posPalmL-posWrist);
		glm::vec3 v = (posPalmR-posWrist);
		
		auto faceNormal = glm::cross(u, v);
		palmNormal = Utils::normalize(faceNormal, glm::vec3(0.f, 0.f, 1.f));
		if(handed == Handedness::RIGHT ) {
			palmNormal *= -1.f;
		}
		palmPos = (posWrist + posPalmL + posPalmR) / 3.f;
		
		palmUp = Utils::normalize(keypoints[9].pos - posWrist, glm::vec3(0.f, 1.f, 0.0f));
		palmSide = Utils::normalize( glm::cross(palmUp, palmNormal), glm::vec3(1.f, 0.f, 0.0f));
	}
}

//-------------------------------------------
shared_ptr<Hand::Finger> Hand::getFinger(const FingerType& atype) {
	if( (int)atype < (int)fingers.size() ) {
		return fingers[atype];
	}
	return std::shared_ptr<Finger>();
}

//-------------------------------------------
glm::vec3& Hand::getFingerTip(const FingerType& atype) {
	if( auto finger = getFinger(atype) ) {
		return finger->getTip();
	}
	return dummyPos;
}

//-------------------------------------------
glm::vec3& Hand::getFingerTipWorld(const FingerType& atype) {
	if( auto finger = getFinger(atype) ) {
		return finger->getTipWorld();
	}
	return dummyPos;
}

//-------------------------------------------
void Hand::drawFingers(bool abUseZ) {
	for( auto& finger : fingers ) {
		finger->draw(abUseZ);
	}
}

//-------------------------------------------
void Hand::drawFingersInfo(bool abUseZ) {
	for( auto& finger : fingers ) {
		finger->drawInfo(abUseZ);
	}
}

//-------------------------------------------
int Hand::getNumFingersOpen() {
	int num = 0;
	for( auto& finger : fingers ) {
		if( finger->isOpen(mMaxDegForOpenFinger) ) {
			num++;
		}
	}
	return num;
}

//-------------------------------------------
int Hand::getNumFingersClosed() {
	int num = 0;
	for( auto& finger : fingers ) {
		if( !finger->isOpen(mMaxDegForOpenFinger) ) {
			num++;
		}
	}
	return num;
}

//-------------------------------------------
bool Hand::isFingerOpen( const FingerType& atype ) {
	int itype = (int)atype;
	if( itype < fingers.size() ) {
		return fingers[itype]->isOpen(mMaxDegForOpenFinger);
	}
	return false;
}



