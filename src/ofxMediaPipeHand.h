//
//  ofxMediaPipeHand.h
//  HandShapeController
//
//  Created by Nick Hardeman on 4/25/24.
//

#pragma once
#include "ofVectorMath.h"
#include "ofMesh.h"
#include <map>
#include "ofxMediaPipeTrackedObject.h"

namespace ofx::MediaPipe {
class Hand : public TrackedObject {
public:
	enum Handedness {
		RIGHT=0,
		LEFT
	};
	
	enum Keypoint_Index {
		WRIST = 0,
		THUMB_CMC,
		THUMB_MCP,
		THUMB_IP,
		THUMB_TIP,
		INDEX_FINGER_MCP,
		INDEX_FINGER_PIP,
		INDEX_FINGER_DIP,
		INDEX_FINGER_TIP,
		MIDDLE_FINGER_MCP,
		MIDDLE_FINGER_PIP,
		MIDDLE_FINGER_DIP,
		MIDDLE_FINGER_TIP,
		RING_FINGER_MCP,
		RING_FINGER_PIP,
		RING_FINGER_DIP,
		RING_FINGER_TIP,
		PINKY_MCP,
		PINKY_PIP,
		PINKY_DIP,
		PINKY_TIP,
		TOTAL
	};
	
	enum FingerType {
		FINGER_THUMB=0,
		FINGER_INDEX,
		FINGER_MIDDLE,
		FINGER_RING,
		FINGER_PINKY,
		FINGER_TOTAL
	};
	
	class Finger {
	public:
		static glm::vec3 defaultVec3;
		
		FingerType type = FINGER_INDEX;
		std::vector<TrackedObject::Keypoint> keypoints;
		std::vector<float> anglesRad;
		std::string name = "";
		float closedAmountRad = 0.0f;
		bool isOpen(float aDegCutoff) {
			return ofRadToDeg(closedAmountRad) < aDegCutoff;
		}
		
		bool isOpen() {
			return ofRadToDeg(closedAmountRad) < mMaxDegForOpenFinger;
		}
		
		ofColor color;
		
		glm::vec3& getTip();
		glm::vec3& getTipWorld();
		
		void draw( bool aBUseZ );
		void drawInfo( bool aBUseZ );
		void drawWorld();
		
		glm::vec3 handPos = {0.f, 0.f, 0.f};
		glm::vec3 handPosWorld = {0.f, 0.f, 0.f};
		
		float timeOpen = 0.f;
		float timeClosed = 0.f;
		
		float mMaxDegForOpenFinger = 60.0f;
		
		unsigned int handID = 0;
	};
	
	// https://developers.google.com/mediapipe/solutions/vision/hand_landmarker
//	static bool shouldRemove( const std::shared_ptr<Hand>& ahand );
	static std::string sGetFingerTypeAsString(const FingerType& aftype);
	virtual TrackedObjectType getType() override { return TrackedObject::HAND; }
	
	void setupFingers();
	void updateFingers();
	
	void updateFrom( std::shared_ptr<Hand>& aother );
	void updateFromHandWithSmoothing( std::shared_ptr<Hand> aother, float pct );
	void updateFromKeypoints() override;
	
	void setMaxDegreesForOpenFinger( float adegrees ) { mMaxDegForOpenFinger = adegrees;}
	float& getMaxDegreesForOpenFinger() { return mMaxDegForOpenFinger;}
	
//	void draw(bool abUseZ);
	void drawFingers(bool abUseZ);
	void drawFingersInfo(bool abUseZ);
	
//	glm::vec3& getPosition() override;
//	glm::vec3& getPositionWorld() override;
//	glm::vec3& getPositionNormalized() override;
	
	int getNumFingersOpen();
	int getNumFingersClosed();
	bool isFingerOpen( const FingerType& atype );
	
//	std::vector<Tracker::Keypoint> keypoints;
	std::vector< std::shared_ptr<Finger> > fingers;
	Handedness handed = Handedness::RIGHT;
	
	std::shared_ptr<Finger> getFinger(const FingerType& atype);
	glm::vec3& getFingerTip(const FingerType& atype);
	glm::vec3& getFingerTipWorld(const FingerType& atype);
	
//	glm::vec3 pos = {0.f, 0.f, 0.f }; // same as wrist position
//	glm::vec3 posN = {0.f, 0.f, 0.f }; // same as wrist position
//	glm::vec3 posWorld = {0.f, 0.f, 0.f }; // same as wrist world position
	
	int index = 0;
	glm::vec3 palmNormal = {0.f, 0.f, 1.f};
	glm::vec3 palmPos = {0.f, 0.f, 0.f};
	glm::vec3 palmUp = {0.f, -1.0f, 0.0f };
	glm::vec3 palmSide = {1.f, 0.0f, 0.0f };
	
//	unsigned int ID = 0;
//	float age = 0.f;
	
//	Tracker::TrackingData trackingData;
//	bool bRemove = false;
	
protected:
	
	float mMaxDegForOpenFinger = 60.0f;
	
};
}
