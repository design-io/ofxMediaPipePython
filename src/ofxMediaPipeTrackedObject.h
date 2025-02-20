//
//  ofxMediaPipeTrackedObject.h
//  PoseTrackerDemo
//
//  Created by Nick Hardeman on 4/30/24.
//

#pragma once
#include "ofVectorMath.h"
#include "ofMesh.h"
#include "ofPolyline.h"
#include "ofJson.h"

namespace ofx::MediaPipe {
class TrackedObject {
public:
	
	enum TrackedObjectType {
		HAND=0,
		FACE,
		POSE
	};
	
	struct Keypoint {
		glm::vec3 pos = {0.f, 0.f, 0.f};
		glm::vec3 posN = {0.f, 0.f, 0.f}; // normalized position
		glm::vec3 posWorld = {0.f, 0.f, 0.f};
		// these don't seem to change based on testing
		//		float visibility = 0.f;
		//		float presence = 0.f;
	};
	
	struct TrackingData {
		std::int64_t mostRecentFrame = 0;
		int numFramesNotFound = 0;
		bool bFoundThisFrame = true;
		float matchDistance = -1.f; // used for match algo
		bool positionsSet = false;
		bool worldPositionsSet = false;
		ofFpsCounter fps;
	};
	
	static std::string sGetTypeAsString( TrackedObjectType atype ) {
		std::string rtypeStr = "HAND";
		if( atype == TrackedObject::FACE ) {
			rtypeStr = "FACE";
		} else if( atype == TrackedObject::POSE ) {
			rtypeStr = "POSE";
		}
		return rtypeStr;
	};
	
	static TrackedObjectType sGetTypeFromString( const std::string& astr ) {
		auto cstr = ofToLower(astr);
		if( cstr == "hand") {
			return TrackedObject::HAND;
		} else if( cstr == "face") {
			return TrackedObject::FACE;
		}
		return TrackedObject::POSE;
	};
	
	static bool shouldRemove( const std::shared_ptr<TrackedObject>& ao ) {
		return (!ao || ao->bRemove);
	}
	
	virtual TrackedObjectType getType() = 0;
	std::string getTypeAsString() { return sGetTypeAsString(getType()); }
	
	virtual void updateKeypointsFromOtherWithSmoothing( std::shared_ptr<TrackedObject> aother, float pct );
	virtual void updateFromKeypoints() { updateDrawMeshes(); }
	
	void updateDrawMeshes();
	ofMesh getMesh( const ofRectangle& arect ) const;
	
	void drawPoints( float aRadius, bool aBUseZ );
	void drawOutlines( bool aBUseZ );
	void drawOutlinesWorld();
	
	void drawPoints( float aRadius, float ax, float ay, float aw, float ah ) const;
	
	glm::vec3& getPosForIndex( const ofIndexType& aindex );
	Keypoint& getKeypointForIndex( const ofIndexType& aindex );
	
	ofPolyline getPolylineForIndices( const std::vector<ofIndexType>& aindices, bool aBUseZ );
	
	virtual glm::vec3 getPosition();
	virtual glm::vec3 getPositionWorld();
	virtual glm::vec3 getPositionNormalized();
	
	virtual ofMesh& getKeypointsLineMesh() { return mKeypointsLineMesh; }
	virtual ofMesh& getKeypointsNormalizedLineMesh() { return mKeypointsLineMeshN; }
	
	float age = 0.f;
	unsigned int ID = 0;
	bool bRemove = false;
	TrackingData trackingData;
	
	
	bool bDrawWithZ = true;
	
	std::vector<Keypoint> keypoints;
	std::vector< glm::vec3 > mVertices, mVertices2d, mVerticesN, mVerticesWorld;

protected:
	void _updateVertices();
	ofMesh mKeypointsLineMesh, mKeypointsLineMeshN, mKeypointsLineMeshWorld;
	
	
	bool mBMeshesHaveZ = true;
	
	glm::vec3 dummyPos = {0.f, 0.f, 0.f };
	Keypoint dummyKeyPoint;
};
}
