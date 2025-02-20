//
//  ofxMediaPipePose.h
//  PoseTrackerDemo
//
//  Created by Nick Hardeman on 4/30/24.
//

#pragma once
#include "ofVectorMath.h"
#include "ofMesh.h"
#include <map>
#include "ofxMediaPipeTrackedObject.h"

namespace ofx::MediaPipe {
class Pose : public TrackedObject {
public:
	// https://developers.google.com/mediapipe/solutions/vision/pose_landmarker
	enum Keypoint_Index {
		NOSE=0,
		LEFT_EYE_INNER, // 1
		LEFT_EYE, // 2
		LEFT_EYE_OUTER, //3
		RIGHT_EYE_INNER, //4
		RIGHT_EYE, //5
		RIGHT_EYE_OUTER,//6
		LEFT_EAR,//7
		RIGHT_EAR,//8
		LEFT_MOUTH,//9
		RIGHT_MOUTH,//10
		LEFT_SHOULDER,//11
		RIGHT_SHOULDER,//12
		LEFT_ELBOW,//13
		RIGHT_ELBOW,//14
		LEFT_WRIST,//15
		RIGHT_WRIST,//16
		LEFT_PINKY,//17
		RIGHT_PINKY,
		LEFT_INDEX,
		RIGHT_INDEX,
		LEFT_THUMB,
		RIGHT_THUMB,
		LEFT_HIP,
		RIGHT_HIP,
		LEFT_KNEE,
		RIGHT_KNEE,
		LEFT_ANKLE,
		RIGHT_ANKLE,
		LEFT_HEEL,
		RIGHT_HEEL,
		LEFT_FOOT_INDEX,
		RIGHT_FOOT_INDEX,
		TOTAL
	};
	
	virtual TrackedObjectType getType() override { return TrackedObject::POSE; }
	
	void updateFrom( std::shared_ptr<Pose>& aother );
	void updateFromPoseWithSmoothing( std::shared_ptr<Pose> aother, float pct );
	
	void updateFromKeypoints() override;
	
	glm::vec3 getPosition() override;
	glm::vec3 getPositionWorld() override;
	glm::vec3 getPositionNormalized() override;
	
	glm::mat3 getTorsoRotation(float aZScale);
	glm::quat getTorsoOrientation(float aZScale);
	glm::mat3 getTorsoRotationWorld(float aZScale);
	glm::quat getTorsoOrientationWorld(float aZScale);
	glm::mat4 getTorsoMatrix(float aZScale);
	glm::mat4 getTorsoMatrixWorld(float aZScale);
	
	glm::quat getHipsOrientation( float aZScale );
	glm::quat getHipsOrientationWorld( float aZScale );
	glm::mat4 getHipsMatrix(float aZScale);
	glm::mat4 getHipsMatrixWorld(float aZScale);
	
	glm::mat3 getFaceRotation(float aZScale);
	glm::quat getFaceOrientation(float aZScale);
	glm::mat3 getFaceRotationWorld(float aZScale);
	glm::quat getFaceOrientationWorld(float aZScale);
	glm::mat4 getFaceMatrix(float aZScale);
	glm::mat4 getFaceMatrixWorld(float aZScale);
	glm::vec3 getFaceUpDirection(float aZScale);
	glm::vec3 getFaceUpDirectionWorld(float aZScale);
	
	glm::quat getFaceOrientationScreen();
	glm::vec3 getFaceDirectionScreen( int aindex ) { getFaceOrientationScreen(); return mFaceDirsScreen[aindex];}
	
	glm::vec3 getShouldersMidPoint(float aZScale);
	glm::vec3 getHipsMidPoint(float aZScale);
	glm::vec3 getShouldersMidPointWorld(float aZScale);
	glm::vec3 getHipsMidPointWorld(float aZScale);
	glm::vec3 getFacePosition(float aZScale);
	glm::vec3 getFacePositionWorld(float aZScale);
	glm::vec3 getFacePositionNormalized();
	
	glm::mat4 getMatrix( const glm::vec3& apos, const glm::mat3& arotMat );
	glm::mat3 getRotationFromQuad( const glm::vec3& ap1, const glm::vec3& ap2, const glm::vec3& ap3, const glm::vec3& ap4, float aZScale, bool aBInverseY );
	glm::quat getRotationFromTri( const glm::vec3& ap1, const glm::vec3& ap2, const glm::vec3& ap3, float aZScale, bool aBInverseY );
	
	void setFlipOrientationZ( bool ab ) { mBSetFlipOrientationZ = ab;}
	
protected:
	bool _isAlignedToScreenUp( const glm::vec3& aup );
	bool mBSetFlipOrientationZ = false;
	
	glm::vec3 mFaceDirsScreen[3];
};
}
