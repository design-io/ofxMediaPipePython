//
//  ofxMediaPipeTrackedObject.cpp
//  PoseTrackerDemo
//
//  Created by Nick Hardeman on 4/30/24.
//

#include "ofxMediaPipeTrackedObject.h"
#include "ofGraphics.h"

using namespace ofx::MediaPipe;

using std::string;
using std::shared_ptr;
using std::vector;
using std::make_shared;
using std::cout;
using std::endl;

//-------------------------------------------
void TrackedObject::updateKeypointsFromOtherWithSmoothing( std::shared_ptr<TrackedObject> aother, float pct ) {
	if( keypoints.size() == 0 ){
		keypoints = aother->keypoints;
	} else {
		for( std::size_t i = 0; i < keypoints.size(); i++ ){
			auto & kp = keypoints[i];
			auto & kpNew = aother->keypoints[i];
			
			kp.pos = glm::mix(kp.pos, kpNew.pos, 1.0f-pct);
			kp.posN = glm::mix(kp.posN, kpNew.posN, 1.0f-pct);
			kp.posWorld = glm::mix(kp.posWorld, kpNew.posWorld, 1.0f-pct);
		}
	}
	updateFromKeypoints();
}

//--------------------------------------------------------------
void TrackedObject::updateDrawMeshes() {
	if( keypoints.size() > 0 ) {
//		auto& vertices = mKeypointsLineMesh.getVertices();
		_updateVertices();
		
		mKeypointsLineMesh.setMode(OF_PRIMITIVE_LINES);
		mKeypointsLineMesh.getVertices() = mVertices;
		
		mKeypointsLineMeshN = mKeypointsLineMesh;
		mKeypointsLineMeshN.getVertices() = mVerticesN;
		
		mKeypointsLineMeshWorld = mKeypointsLineMesh;
		mKeypointsLineMeshWorld.getVertices() = mVerticesWorld;
		
	} else {
		mKeypointsLineMesh.clear();
	}
	mBMeshesHaveZ = true;
}

//-------------------------------------------
ofMesh TrackedObject::getMesh( const ofRectangle& arect ) const {
	ofMesh rmesh = mKeypointsLineMeshN;
	for( auto& v : rmesh.getVertices() ) {
		v.x *= arect.width;
		v.y *= arect.height;
		v.x += arect.x;
		v.y += arect.y;
	}
	return rmesh;
}

//-------------------------------------------
void TrackedObject::drawPoints( float aRadius, bool aBUseZ ) {	
	for( auto& pt : keypoints ) {
		if( aBUseZ ) {
			ofDrawCircle( pt.pos, aRadius );
		} else {
			ofDrawCircle( pt.pos.x, pt.pos.y, aRadius );
		}
	}
}

//-------------------------------------------
void TrackedObject::drawOutlines( bool aBUseZ ) {
	if( aBUseZ ) {
		if( !mBMeshesHaveZ ) {
			mKeypointsLineMesh.getVertices() = mVertices;
		}
	} else {
		if( mBMeshesHaveZ ) {
			mKeypointsLineMesh.getVertices() = mVertices2d;
		}
	}
	mBMeshesHaveZ = aBUseZ;
	mKeypointsLineMesh.draw();
}

//-------------------------------------------
void TrackedObject::drawOutlinesWorld() {
	mKeypointsLineMeshWorld.draw();
}

//-------------------------------------------
void TrackedObject::drawPoints(float aRadius, float ax, float ay, float aw, float ah ) const {
	for( auto& kp : keypoints ) {
		glm::vec2 np = kp.posN;
		np.x *= aw;
		np.y *= ah;
		np.x += ax;
		np.y += ay;
		if( np.x < ax ) continue;
		if( np.y < ay ) continue;
		if( np.x > ax + aw ) continue;
		if( np.y > ay+ah) continue;
		ofDrawCircle( np, aRadius );
	}
}

//-------------------------------------------
glm::vec3& TrackedObject::getPosForIndex( const ofIndexType& aindex ) {
	if( mVertices.size() == 0 ){
		_updateVertices(); 
	}
	if( mVertices.size() < 2 ) {
		return dummyPos;
	}
	return mVertices[aindex];
}

//-------------------------------------------
TrackedObject::Keypoint& TrackedObject::getKeypointForIndex( const ofIndexType& aindex ) {
	if( keypoints.size() < 2 ) {
		return dummyKeyPoint;
	}
	return keypoints[aindex];
}

//-------------------------------------------
glm::vec3 TrackedObject::getPosition() {
	if( keypoints.size() > 0 ) {
		return keypoints[0].pos;
	}
	return dummyPos;
}

//-------------------------------------------
glm::vec3 TrackedObject::getPositionWorld() {
	if( keypoints.size() > 0 ) {
		return keypoints[0].posWorld;
	}
	return dummyPos;
}

//-------------------------------------------
glm::vec3 TrackedObject::getPositionNormalized() {
	if( keypoints.size() > 0 ) {
		return keypoints[0].posN;
	}
	return dummyPos;
}

//-------------------------------------------
ofPolyline TrackedObject::getPolylineForIndices( const std::vector<ofIndexType>& aindices, bool aBUseZ ) {
	size_t numKeypoints = keypoints.size();
	ofPolyline pline;
	if( numKeypoints < 2 ) return pline;
	
	glm::vec3 tv = {0.f, 0.f, 0.f};
	for( auto& index : aindices ) {
		tv = keypoints[index].pos;
		if( !aBUseZ ) {
			tv.z = 0.f;
		}
		pline.addVertex( tv );
	}
	return pline;
}

//-------------------------------------------
void TrackedObject::_updateVertices() {
	if(mVertices.size() != keypoints.size() ) {
		mVertices.assign(keypoints.size(), glm::vec3(0.f, 0.f, 0.f ));
		mVertices2d.assign(keypoints.size(), glm::vec3(0.f, 0.f, 0.f ));
		mVerticesN.assign(keypoints.size(), glm::vec3(0.f, 0.f, 0.f ));
		mVerticesWorld.assign(keypoints.size(), glm::vec3(0.f, 0.f, 0.f ));
	}
	
	size_t numKeypoints = keypoints.size();
	for( size_t i = 0; i < numKeypoints; i++ ) {
		mVertices[i] = keypoints[i].pos;
		mVertices2d[i] = mVertices[i];
		mVertices2d[i].z = 0.f;
		mVerticesN[i] = keypoints[i].posN;
		mVerticesWorld[i] = keypoints[i].posWorld;
	}
}


