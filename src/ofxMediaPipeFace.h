//
//  ofxMediaPipeFace.h
//  FaceControls
//
//  Created by Nick Hardeman on 4/24/24.
//

#pragma once
#include "ofMesh.h"
#include <map>
#include "ofxMediaPipeTrackedObject.h"

namespace ofx::MediaPipe {
class Face : public TrackedObject {
public:
	
	// https://storage.googleapis.com/mediapipe-assets/Model%20Card%20Blendshape%20V2.pdf
	enum BlendShapeType {
		NEUTRAL=0,
		BROW_DOWN_LEFT,
		BROW_DOWN_RIGHT,
		BROW_INNER_UP,
		BROW_OUTER_UP_LEFT,
		BROW_OUTER_UP_RIGHT,
		CHEEK_PUFF,
		CHEEK_SQUINT_LEFT,
		CHEEK_SQUINT_RIGHT,
		EYE_BLINK_LEFT,
		EYE_BLINK_RIGHT,
		EYE_LOOK_DOWN_LEFT,
		EYE_LOOK_DOWN_RIGHT,
		EYE_LOOK_IN_LEFT,
		EYE_LOOK_IN_RIGHT,
		EYE_LOOK_OUT_LEFT,
		EYE_LOOK_OUT_RIGHT,
		EYE_LOOK_UP_LEFT,
		EYE_LOOK_UP_RIGHT,
		EYE_SQUINT_LEFT,
		EYE_SQUINT_RIGHT,
		EYE_WIDE_LEFT,
		EYE_WIDE_RIGHT,
		JAW_FORWARD,
		JAW_LEFT,
		JAW_OPEN,
		JAW_RIGHT,
		MOUTH_CLOSE,
		MOUTH_DIMPLE_LEFT,
		MOUTH_DIMPLE_RIGHT,
		MOUTH_FROWN_LEFT,
		MOUTH_FROWN_RIGHT,
		MOUTH_FUNNEL,
		MOUTH_LEFT,
		MOUTH_LOWER_DOWN_LEFT,
		MOUTH_LOWER_DOWN_RIGHT,
		MOUTH_PRESS_LEFT,
		MOUTH_PRESS_RIGHT,
		MOUTH_PUCKER,
		MOUTH_RIGHT,
		MOUTH_ROLL_LOWER,
		MOUTH_ROLL_UPPER,
		MOUTH_SHRUG_LOWER,
		MOUTH_SHRUG_UPPER,
		MOUTH_SMILE_LEFT,
		MOUTH_SMILE_RIGHT,
		MOUTH_STRETCH_LEFT,
		MOUTH_STRETCH_RIGHT,
		MOUTH_UPPER_UP_LEFT,
		MOUTH_UPPER_UP_RIGHT,
		NOSE_SNEER_LEFT,
		NOSE_SNEER_RIGHT,
		TONGUE_OUT,
		TOTAL,
		UNKNOWN
	};
	
	enum Keypoint_Index {
		KP_EYE_RIGHT_OUTER=33,
		KP_EYE_RIGHT_INNER=133,
		KP_EYE_RIGHT_MID_TOP=159,
		KP_EYE_RIGHT_MID_BOTTOM=145,
		
		KP_EYE_LEFT_OUTER=263,
		KP_EYE_LEFT_INNER=362,
		KP_EYE_LEFT_MID_TOP=386,
		KP_EYE_LEFT_MID_BOTTOM=374,
		
		KP_EYE_IRIS_RIGHT=468,
		KP_EYE_IRIS_LEFT=473,
		
		KP_NOSE_TOP=168,
		KP_NOSE_TIP=4,
		KP_NOSE_BOTTOM=2,
		
		KP_MOUTH_OUTER_LEFT=291,
		KP_MOUTH_OUTER_RIGHT=61,
		KP_MOUTH_OUTER_MID_TOP=0,
		KP_MOUTH_OUTER_MID_BOTTOM=17,
		
		KP_MOUTH_INNER_LEFT=78,
		KP_MOUTH_INNER_RIGHT=308,//324,
		KP_MOUTH_INNER_MID_TOP=13,
		KP_MOUTH_INNER_MID_BOTTOM=14,
		
		KP_CHIN=152,
		KP_FOREHEAD=10,
		KP_EAR_RIGHT=234,
		KP_EAR_LEFT=454,
		
		KP_EYEBROW_RIGHT_INNER_TOP=107,
		KP_EYEBROW_RIGHT_INNER_BOTTOM=55,
		KP_EYEBROW_RIGHT_OUTER_TOP=70,
		KP_EYEBROW_RIGHT_OUTER_BOTTOM=46,
		
		KP_EYEBROW_LEFT_INNER_TOP=336,
		KP_EYEBROW_LEFT_INNER_BOTTOM=285,
		KP_EYEBROW_LEFT_OUTER_TOP=300,
		KP_EYEBROW_LEFT_OUTER_BOTTOM=276
	};
	
//	https://storage.googleapis.com/mediapipe-assets/Model%20Card%20MediaPipe%20Face%20Mesh%20V2.pdf
	
	struct BlendShape {
		BlendShapeType type = NEUTRAL;
		std::string category_name = "neutral";
		float score = 0.0f;
		int index = 0;
		float timeActive = 0.0f;
		float timeInActive = 0.0f;
		bool bFiredActiveEvent = false;
		bool bChangedToActive = false;
	};
	
	
	
	virtual TrackedObjectType getType() override { return TrackedObject::FACE; }
	
	static int sGetNumTrackedLandmarks() { return 478; }
	static BlendShapeType sGetBlendShape( const std::string& abtype );
	static std::string sGetBlendShapeAsString( BlendShapeType abtype );
	
//	static bool shouldRemove( const std::shared_ptr<Face>& aface );
	static bool sortBlendShapesOnIndex( const BlendShape& aa, const BlendShape& ab );
	static bool sortBlendShapesOnScore( const BlendShape& aa, const BlendShape& ab );
	
	void updateFrom( std::shared_ptr<Face>& aother );
	void updateFromFaceWithSmoothing( std::shared_ptr<Face>& aother, float pct );
	
	void updateFromKeypoints() override;
	void updateBlendShapes(const std::vector< Face::BlendShape >& aOtherBlendShapes );
	
	// called from face tracker
	void setIncomingBlendShape( const std::string& aCatName, const float& ascore, const int& aindex );
	
	std::vector< BlendShape >& getIncomingBlendShapes();
	std::vector< BlendShape >& getBlendShapes();
	
	BlendShape& getBlendShape( const std::string& acategory );
	BlendShape& getBlendShape( const BlendShapeType& atype );
	std::vector<BlendShape> getTopBlendShapes( int aTopToReturn );
	
	void drawIrises(float aRadius, bool aBUseZ );
	
	// https://storage.googleapis.com/mediapipe-assets/documentation/mediapipe_face_landmark_fullsize.png
	std::vector<ofIndexType> getEyebrowIndices(bool bRight);
	std::vector<ofIndexType> getEyebrowIndicesTop(bool bRight);
	std::vector<ofIndexType> getEyebrowIndicesBottom(bool bRight);
	
	std::vector<ofIndexType> getEyeIndices(bool bRight);
	std::vector<ofIndexType> getEyeIndicesTop(bool bRight);
	std::vector<ofIndexType> getEyeIndicesBottom(bool bRight);
	
	std::vector<ofIndexType> getEyeIrisIndices(bool bRight);
	std::vector<ofIndexType> getOuterMouthIndices();
	std::vector<ofIndexType> getInnerMouthIndices();

	std::vector<ofIndexType> getInnerMouthIndicesTop();
	std::vector<ofIndexType> getInnerMouthIndicesBottom();


	std::vector<ofIndexType> getOutlineIndices();
	
	glm::vec3 getPosition() override;
	glm::vec3 getPositionWorld() override;
	glm::vec3 getPositionNormalized() override;
	
	ofIndexType getIrisCenterIndex( bool bRight );
	glm::vec3& getRightIrisPos();
	glm::vec3& getLeftIrisPos();
		
	bool isRightEyeBlinking();
	bool isLeftEyeBlinking();
	
	float getEyeOpenPercent(bool bRight);
	
	float getMouthOpenPercent();
	float getEyebrowRaisePercent(bool bRight);
	
	glm::quat getOrientation() { return mOrientation;}
	glm::quat getScreenOrientation() { return mScreenOrientation;}
	glm::vec3 getSideDir() { return mFaceDirs[0]; }
	glm::vec3 getUpDir() { return mFaceDirs[1]; }
	glm::vec3 getForwardDir() { return mFaceDirs[2]; }
	
	glm::vec3 getSideDirScreen() { return mFaceScreenDirs[0]; }
	glm::vec3 getUpDirScreen() { return mFaceScreenDirs[1]; }
	glm::vec3 getForwardDirScreen() { return mFaceScreenDirs[2]; }
	glm::vec3 getFaceScreenDirection(int aindex) { return mFaceScreenDirs[aindex]; }
	
protected:
	static void sCreateBlendShapeStringMap();
	void _addToLinesMesh( ofMesh& amesh, const std::vector<ofIndexType>& aindices, ofFloatColor acolor );
	void _updateAxes();
	
//	ofMesh mLinesMesh, mLinesMesh2d;
	
	std::vector<BlendShape> mIncomingBlendShapes;
	std::vector<BlendShape> mBlendShapes;
	
	static std::map<std::string, int> sBlendShapeStringMap;
	
	BlendShape dummyShape;
	
	glm::quat mOrientation = glm::vec3(0.f, 0.f, 0.f);
	glm::quat mScreenOrientation = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 mFaceDirs[3] = {{0.0f, 0.0f, 1.}, {0.0f, 0.0f, 1.}, {0.0f, 0.0f, 1.}};
	glm::vec3 mFaceScreenDirs[3] = {{0.0f, 0.0f, -1.}, {0.0f, 0.0f, -1.}, {0.0f, 0.0f, -1.}};
	
//	glm::vec3 dummyPos = {0.f, 0.f, 0.f };
	
};
}
