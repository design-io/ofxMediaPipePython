//
//  ofxMediaPipePose.cpp
//  PoseTrackerDemo
//
//  Created by Nick Hardeman on 4/30/24.
//

#include "ofxMediaPipePose.h"
#include "ofxMediaPipeUtils.h"

using namespace ofx::MediaPipe;

using std::string;
using std::shared_ptr;
using std::vector;
using std::make_shared;
using std::cout;
using std::endl;

//-------------------------------------------
void Pose::updateFrom( std::shared_ptr<Pose>& aother ) {
	keypoints = aother->keypoints;
	updateFromKeypoints();
}

//-------------------------------------------
void Pose::updateFromPoseWithSmoothing( std::shared_ptr<Pose> aother, float pct ) {
	updateKeypointsFromOtherWithSmoothing( aother, pct );
//	if( keypoints.size() == 0 ){
//		keypoints = aother->keypoints;
//	}else{
//		for( std::size_t i = 0; i < keypoints.size(); i++ ){
//			auto & kp = keypoints[i];
//			auto & kpNew = aother->keypoints[i];
//			
//			kp.pos = glm::mix(kp.pos, kpNew.pos, 1.0f-pct);
//			kp.posN = glm::mix(kp.posN, kpNew.posN, 1.0f-pct);
//			kp.posWorld = glm::mix(kp.posWorld, kpNew.posWorld, 1.0f-pct);
//		}
//	}
//	updateFromKeypoints();
}

//-------------------------------------------
void Pose::updateFromKeypoints() {
	
	updateDrawMeshes();
	
	if( mKeypointsLineMesh.getNumIndices() < 1 && keypoints.size() > 1 ) {
		mKeypointsLineMesh.addIndices({
			// eyes
			8, 5,
			5, 0,
			0, 2,
			2, 7,
			// mouth
			10, 9,
			// right arm
			12, 14,
			14, 16,
			16, 18,
			18, 20,
			20, 16,
			16, 22,
			// left arm
			11, 13,
			13, 15,
			15, 17,
			17, 19,
			19, 15,
			15, 21,
			// torso
			12, 11,
			11, 23,
			23, 24,
			24, 12,
			// right leg
			24, 26,
			26, 28,
			28, 32,
			32, 30,
			30, 28,
			// left leg
			23, 25,
			25, 27,
			27, 31,
			31, 29,
			29, 27
		});
	}
};


//-------------------------------------------
glm::vec3 Pose::getPosition() {
	if( keypoints.size() > 23 ) {
		return (keypoints[LEFT_SHOULDER].pos + keypoints[RIGHT_SHOULDER].pos +
				keypoints[LEFT_HIP].pos + keypoints[RIGHT_HIP].pos) * 0.25f;
	}
	return dummyPos;
}

//-------------------------------------------
glm::vec3 Pose::getPositionWorld() {
	if( keypoints.size() > 23 ) {
		// add up torso positions
		return (keypoints[LEFT_SHOULDER].posWorld + keypoints[RIGHT_SHOULDER].posWorld+
				keypoints[LEFT_HIP].posWorld + keypoints[RIGHT_HIP].posWorld) * 0.25f;
	}
	return dummyPos;
}

//-------------------------------------------
glm::vec3 Pose::getPositionNormalized() {
	if( keypoints.size() > 23 ) {
		return (keypoints[LEFT_SHOULDER].posN + keypoints[RIGHT_SHOULDER].posN +
				keypoints[LEFT_HIP].posN + keypoints[RIGHT_HIP].posN) * 0.25f;
	}
	return dummyPos;
}

//-------------------------------------------
glm::mat3 Pose::getTorsoRotation(float aZScale) {
	glm::mat3 m;
	if( keypoints.size() < RIGHT_HIP ) {
		m[0] = glm::vec3(1.f, 0.f, 0.f);
		m[1] = glm::vec3(0.f, 1.f, 0.f);
		m[2] = glm::vec3(0.f, 0.f, 1.f);
		return m;
	}
	
	// now lets update the torso rotation
	auto p1 = keypoints[LEFT_SHOULDER].pos;
	auto p2 = keypoints[RIGHT_SHOULDER].pos;
	auto p3 = keypoints[LEFT_HIP].pos;
	auto p4 = keypoints[RIGHT_HIP].pos;
	
	return getRotationFromQuad(p1, p2, p3, p4, aZScale, _isAlignedToScreenUp( p1-p3 ) );
}

//-------------------------------------------
glm::quat Pose::getTorsoOrientation(float aZScale) {
	return glm::toQuat(getTorsoRotation(aZScale));
}

//-------------------------------------------
glm::mat3 Pose::getTorsoRotationWorld(float aZScale) {
	glm::mat3 m;
	if( keypoints.size() < RIGHT_HIP ) {
		m[0] = glm::vec3(1.f, 0.f, 0.f);
		m[1] = glm::vec3(0.f, 1.f, 0.f);
		m[2] = glm::vec3(0.f, 0.f, 1.f);
		return m;
	}
	
	// now lets update the torso rotation
	auto p1 = keypoints[LEFT_SHOULDER].posWorld;
	auto p2 = keypoints[RIGHT_SHOULDER].posWorld;
	auto p3 = keypoints[LEFT_HIP].posWorld;
	auto p4 = keypoints[RIGHT_HIP].posWorld;
	
	return getRotationFromQuad(p1, p2, p3, p4, aZScale, mBSetFlipOrientationZ);
}

//-------------------------------------------
glm::quat Pose::getTorsoOrientationWorld(float aZScale) {
	return glm::toQuat(getTorsoRotationWorld(aZScale));
}

//-------------------------------------------
glm::mat4 Pose::getTorsoMatrix(float aZScale) {
	glm::mat4 lmat = glm::translate(glm::mat4(1.0f), getPosition());
	lmat = lmat * glm::toMat4((const glm::quat&)getTorsoRotation(aZScale));
	return lmat;
}

//-------------------------------------------
glm::mat4 Pose::getTorsoMatrixWorld(float aZScale) {
	glm::mat4 lmat = glm::translate(glm::mat4(1.0f), getPositionWorld());
	lmat = lmat * glm::toMat4((const glm::quat&)getTorsoRotationWorld(aZScale));
	return lmat;
}

//-------------------------------------------
glm::quat Pose::getHipsOrientation( float aZScale ) {
	auto p1 = getShouldersMidPoint(1.f);
	auto p2 = keypoints[LEFT_HIP].pos;
	auto p3 = keypoints[RIGHT_HIP].pos;
	
	return (getRotationFromTri(p1, p2, p3, aZScale, mBSetFlipOrientationZ));
}

//-------------------------------------------
glm::quat Pose::getHipsOrientationWorld( float aZScale ) {
	auto p1 = getShouldersMidPointWorld(1.f);
	auto p2 = getHipsMidPointWorld(1.f);//keypoints[LEFT_HIP].posWorld;
	auto p3 = keypoints[RIGHT_HIP].posWorld;
	
	return (getRotationFromTri(p1, p2, p3, aZScale, mBSetFlipOrientationZ));
}

//-------------------------------------------
glm::mat4 Pose::getHipsMatrix(float aZScale) {
	glm::mat4 lmat = glm::translate(glm::mat4(1.0f), getHipsMidPoint(aZScale));
	lmat = lmat * glm::toMat4(getHipsOrientation(aZScale));
	return lmat;
}

//-------------------------------------------
glm::mat4 Pose::getHipsMatrixWorld(float aZScale) {
	glm::mat4 lmat = glm::translate(glm::mat4(1.0f), getHipsMidPointWorld(aZScale));
	lmat = lmat * glm::toMat4(getHipsOrientationWorld(aZScale));
	return lmat;
}

//-------------------------------------------
glm::mat3 Pose::getFaceRotation(float aZScale) {
	glm::mat3 m;
	if( keypoints.size() < RIGHT_HIP ) {
		m[0] = glm::vec3(1.f, 0.f, 0.f);
		m[1] = glm::vec3(0.f, 1.f, 0.f);
		m[2] = glm::vec3(0.f, 0.f, 1.f);
		return m;
	}
	auto p1 = keypoints[LEFT_EAR].pos;
	auto p2 = keypoints[RIGHT_EAR].pos;
	
	auto p3 = (p1+p2) * 0.5f;
	auto p4 = getShouldersMidPoint(1.f);
	
	return getRotationFromQuad(p1, p2, p3, p4, aZScale, _isAlignedToScreenUp( p1-p4 ));
}

//-------------------------------------------
glm::quat Pose::getFaceOrientation(float aZScale) {
	return glm::toQuat(getFaceRotation(aZScale));
}

//-------------------------------------------
glm::mat3 Pose::getFaceRotationWorld(float aZScale) {
	glm::mat3 m;
	if( keypoints.size() < RIGHT_HIP ) {
		m[0] = glm::vec3(1.f, 0.f, 0.f);
		m[1] = glm::vec3(0.f, 1.f, 0.f);
		m[2] = glm::vec3(0.f, 0.f, 1.f);
		return m;
	}
	
	// now lets update the face rotation
	auto p1 = keypoints[LEFT_EAR].posWorld;
	auto p2 = keypoints[RIGHT_EAR].posWorld;
	auto p3 = keypoints[LEFT_EYE_INNER].posWorld;
	auto p4 = keypoints[RIGHT_EYE_INNER].posWorld;
//	auto p3 = keypoints[MOUTH_LEFT].posWorld;
//	auto p4 = keypoints[MOUTH_RIGHT].posWorld;
	// we need to project the mouth positions to be on the same plane as the ear segment
//	auto diffToEars = glm::normalize(p1-p3);
//	auto diffMouth = glm::normalize(p4-p3);
	// rotate around the x axis, otherwise it's pointing straight up //
	auto xrot = glm::angleAxis(glm::half_pi<float>(), glm::vec3(1.f, 0., 0.f));
	
	return (getRotationFromQuad(p1, p2, p3, p4, aZScale, mBSetFlipOrientationZ) * glm::toMat3((const glm::quat&)xrot) );
}

//-------------------------------------------
glm::mat4 Pose::getFaceMatrix(float aZScale) {
	glm::mat4 lmat = glm::translate(glm::mat4(1.0f), getFacePosition(aZScale));
	lmat = lmat * glm::toMat4((const glm::quat&)getFaceOrientation(aZScale));
	return lmat;
}

//-------------------------------------------
glm::mat4 Pose::getFaceMatrixWorld(float aZScale) {
	glm::mat4 lmat = glm::translate(glm::mat4(1.0f), getFacePositionWorld(aZScale));
	lmat = lmat * glm::toMat4((const glm::quat&)getFaceOrientationWorld(aZScale));
	return lmat;
}

//-------------------------------------------
glm::quat Pose::getFaceOrientationWorld(float aZScale) {
	return glm::toQuat(getFaceRotationWorld(aZScale));
}

//-------------------------------------------
glm::vec3 Pose::getFaceUpDirection(float aZScale) {
	if( keypoints.size() < RIGHT_HIP ) {
		return dummyPos;
	}
	
	auto p1 = keypoints[LEFT_EAR].pos;
	auto p2 = keypoints[RIGHT_EAR].pos;
	
	auto p3 = (p1+p2) * 0.5f;
	auto p4 = getShouldersMidPoint(aZScale);
	
	return Utils::normalize(p3-p4);
}

//-------------------------------------------
glm::vec3 Pose::getFaceUpDirectionWorld(float aZScale) {
	if( keypoints.size() < RIGHT_HIP ) {
		return dummyPos;
	}
	
	auto p1 = keypoints[LEFT_EAR].posWorld;
	auto p2 = keypoints[RIGHT_EAR].posWorld;
	
	auto p3 = (p1+p2) * 0.5f;
	auto p4 = getShouldersMidPointWorld(aZScale);
	
	return Utils::normalize(p3-p4);
}

//-------------------------------------------
glm::quat Pose::getFaceOrientationScreen() {
	auto earLeft = keypoints[LEFT_EAR].pos;
	auto earRight = keypoints[RIGHT_EAR].pos;
	
	auto earMid = earLeft * 0.5f + earRight * 0.5f;
	auto nose = keypoints[NOSE].pos;
	auto eyeBrowMid = keypoints[LEFT_EYE].pos * 0.5f + keypoints[RIGHT_EYE].pos * 0.5f;
	auto mouthMid = keypoints[LEFT_MOUTH].pos * 0.5f + keypoints[RIGHT_MOUTH].pos * 0.5f;
	
	glm::vec3 sideNorm = Utils::normalize(earLeft-earRight, glm::vec3(0.f, 1.f, 0.0f));
//	sideNorm.z *= -1.f;
//	glm::vec3 upNorm = glm::normalize(eyeBrowMid-mouthMid);
//	upNorm.z *= -1.f;
	
//	glm::vec3 projection = glm::dot(upNorm, sideNorm) * sideNorm;
//	upNorm = glm::normalize(upNorm - projection);
	
//	glm::vec3 forward = glm::normalize(glm::cross(upNorm, sideNorm));
	
	glm::vec3 forward = Utils::normalize(eyeBrowMid-earMid, glm::vec3(0.f, 0.f, 1.f));
	
	glm::vec3 projection = glm::dot(forward, sideNorm) * sideNorm;
	forward = Utils::normalize(forward - projection, glm::vec3(0.f, 0.f, 1.f));
	
	glm::vec3 upNorm = glm::cross(forward, sideNorm );
	
	// Recompute side to ensure it's orthogonal to both up and forward
	sideNorm = Utils::normalize(glm::cross(upNorm,forward), glm::vec3(1.f, 0.0f, 0.0f));
	
	mFaceDirsScreen[0] = sideNorm;
	mFaceDirsScreen[1] = upNorm;
	mFaceDirsScreen[2] = forward;
	
	// Create a rotation matrix from the three vectors
	glm::mat3 rotationMatrix(
							 sideNorm,     // X-axis (side vector)
							 upNorm,       // Y-axis (up vector)
							 forward       // Z-axis (forward vector)
							 );
	
	
	auto rotQ = glm::quat_cast(rotationMatrix);
	//	glm::quat rightHandedQuat(rotQ.w, -rotQ.x, (rotQ.y), -rotQ.z);
	
	return rotQ;
}

////-------------------------------------------
//glm::vec3 Pose::getFacePosition() {
//	auto p1 = keypoints[LEFT_EAR].pos;
//	auto p2 = keypoints[RIGHT_EAR].pos;
//	auto p3 = keypoints[MOUTH_LEFT].pos;
//	auto p4 = keypoints[MOUTH_RIGHT].pos;
//	return (p1 + p2 + p3 + p4) * 0.25;
//}
//
////-------------------------------------------
//glm::vec3 Pose::getFacePositionWorld() {
//	auto p1 = keypoints[LEFT_EAR].posWorld;
//	auto p2 = keypoints[RIGHT_EAR].posWorld;
//	auto p3 = keypoints[MOUTH_LEFT].posWorld;
//	auto p4 = keypoints[MOUTH_RIGHT].posWorld;
//	return (p1 + p2 + p3 + p4) * 0.25;
//}

////-------------------------------------------
//glm::mat4 Pose::getFaceMatrix(float aZScale) {
//	glm::mat4 lmat = glm::translate(glm::mat4(1.0f), getPosition());
//	lmat = lmat * glm::toMat4((const glm::quat&)getTorsoRotation(aZScale));
//	return lmat;
//}

//-------------------------------------------
glm::vec3 Pose::getShouldersMidPoint(float aZScale) {
	auto rpos = (keypoints[LEFT_SHOULDER].pos + keypoints[RIGHT_SHOULDER].pos) * 0.5f;
	rpos.z *= aZScale;
	return rpos;
}

//-------------------------------------------
glm::vec3 Pose::getHipsMidPoint(float aZScale) {
	auto rpos =  (keypoints[LEFT_HIP].pos + keypoints[RIGHT_HIP].pos) * 0.5f;
	rpos.z *= aZScale;
	return rpos;
}

//-------------------------------------------
glm::vec3 Pose::getShouldersMidPointWorld(float aZScale) {
	auto rpos = (keypoints[LEFT_SHOULDER].posWorld + keypoints[RIGHT_SHOULDER].posWorld) * 0.5f;
	rpos.z *= aZScale;
	return rpos;
}

//-------------------------------------------
glm::vec3 Pose::getHipsMidPointWorld(float aZScale) {
	auto rpos = (keypoints[LEFT_HIP].posWorld + keypoints[RIGHT_HIP].posWorld) * 0.5f;
	rpos.z *= aZScale;
	return rpos;
}

//-------------------------------------------
glm::vec3 Pose::getFacePosition(float aZScale) {
	auto rpos =  (keypoints[LEFT_EAR].pos + keypoints[RIGHT_EAR].pos) * 0.5f;
	rpos.z *= aZScale;
	return rpos;
}

//-------------------------------------------
glm::vec3 Pose::getFacePositionWorld(float aZScale) {
	auto rpos = (keypoints[LEFT_EAR].posWorld + keypoints[RIGHT_EAR].posWorld) * 0.5f;
	rpos.z *= aZScale;
	return rpos;
}

//-------------------------------------------
glm::vec3 Pose::getFacePositionNormalized() {
	auto rpos =  (keypoints[LEFT_EAR].posN + keypoints[RIGHT_EAR].posN) * 0.5f;
	return rpos;
}

//-------------------------------------------
glm::mat4 Pose::getMatrix( const glm::vec3& apos, const glm::mat3& arotMat ) {
	glm::mat4 lmat = glm::translate(glm::mat4(1.0f), apos );
	lmat = lmat * glm::toMat4((const glm::quat&)arotMat);
	return lmat;
}

//-------------------------------------------
glm::mat3 Pose::getRotationFromQuad( const glm::vec3& ap1, const glm::vec3& ap2, const glm::vec3& ap3, const glm::vec3& ap4, float aZScale, bool aBInverseY ) {
	
	auto p1 = glm::vec3( ap1.x, ap1.y, ap1.z * aZScale );
	auto p2 = glm::vec3( ap2.x, ap2.y, ap2.z * aZScale );
	auto p3 = glm::vec3( ap3.x, ap3.y, ap3.z * aZScale );
	auto p4 = glm::vec3( ap4.x, ap4.y, ap4.z * aZScale );
	
	glm::vec3 v1 = (p2-p1); // top side vector
	glm::vec3 v2 = (p4-p3); // bottom side vector
	glm::vec3 u1 = (p1-p3); // first up vector
	glm::vec3 u2 = (p2-p4); // second up vector
	
	glm::vec3 v = glm::mix(glm::normalize(v1), glm::normalize(v2), 0.5f);
	glm::vec3 u = glm::mix(glm::normalize(u1), glm::normalize(u2), 0.5f);
	
	if( !Utils::isValid(v)) {
		v = glm::vec3(1.f, 0.0f, 0.0f );
	}
	if( !Utils::isValid(u)) {
		u = glm::vec3(0.f, 1.f, 0.0f);
	}
	
	// TODO: ADD ABILITY TO INVERT ROTATIONS
	auto faceNormal = Utils::normalize(glm::cross(u, v), glm::vec3(0.f, 0.f, 1.f));
	auto up = u;
	auto side = v;
	glm::mat3 m;
	m[0] = -side;
	m[1] = up;
	m[2] = faceNormal;
	if(aBInverseY) {
		m[0] = side;
		m[1] = up;
		m[2] = -faceNormal;
	}
	return m;
}

//-------------------------------------------
glm::quat Pose::getRotationFromTri( const glm::vec3& ap1, const glm::vec3& ap2, const glm::vec3& ap3, float aZScale, bool aBInverseY ) {
	
	auto p1 = glm::vec3( ap1.x, ap1.y, ap1.z * aZScale );
	auto p2 = glm::vec3( ap2.x, ap2.y, ap2.z * aZScale );
	auto p3 = glm::vec3( ap3.x, ap3.y, ap3.z * aZScale );
	
//	glm::vec3 v1 = (p1-p2); // top side vector
//	glm::vec3 v2 = (p3-p2); // bottom side vector
	
	glm::vec3 v = Utils::normalize(p2-p1, glm::vec3(1.f, 0.f, 0.f));//glm::mix(glm::normalize(v1), glm::normalize(v2), 0.5f);
	glm::vec3 u = Utils::normalize(p3-p2, glm::vec3(0.f, 1.f, 0.f));//glm::mix(glm::normalize(u1), glm::normalize(u2), 0.5f);
	
	// TODO: ADD ABILITY TO INVERT ROTATIONS
	auto faceNormal = Utils::normalize(glm::cross(u, v), glm::vec3(0.f, 0.f, 1.f));
	auto up = v;
	auto side = u;
	
//	return glm::rotation(glm::vec3(0.f, 0.f, 1.f), faceNormal );
	glm::mat3 m;
	m[0] = -side;
	m[1] = -up;
	m[2] = faceNormal;
	if(aBInverseY) {
		m[0] = side;
		m[1] = -up;
		m[2] = -faceNormal;
	}
	
//	m[0] = -side;
//	m[1] = -up;
//	m[2] = faceNormal;
//	if(aBInverseY) {
//		m[0] = side;
//		m[1] = -up;
//		m[2] = -faceNormal;
//	}
	return m;
}

//-------------------------------------------
bool Pose::_isAlignedToScreenUp( const glm::vec3& aup ) {
	// check for screen space up
	if( glm::length2(aup) < glm::epsilon<float>()) {
		return false;
	}
	return ( glm::dot( glm::vec3(0.f, -1.f, 0.f), glm::normalize(aup) ) > 0.0f );
}

