//
//  ofxMediaPipeFace.cpp
//  FaceControls
//
//  Created by Nick Hardeman on 4/24/24.
//

#include "ofxMediaPipeFace.h"
#include "ofGraphics.h"
#include "ofxMediaPipeUtils.h"

using namespace ofx::MediaPipe;

std::map<std::string, int> Face::sBlendShapeStringMap;

//-------------------------------------------
void Face::sCreateBlendShapeStringMap() {
	sBlendShapeStringMap["_neutral"] = -1;
	sBlendShapeStringMap["browDownLeft"] = 0;
	sBlendShapeStringMap["browDownRight"] = 1;
	sBlendShapeStringMap["browInnerUp"] = 2;
	sBlendShapeStringMap["browOuterUpLeft"] = 3;
	sBlendShapeStringMap["browOuterUpRight"] = 4;
	sBlendShapeStringMap["cheekPuff"] = 5;
	sBlendShapeStringMap["cheekSquintLeft"] = 6;
	sBlendShapeStringMap["cheekSquintRight"] = 7;
	sBlendShapeStringMap["eyeBlinkLeft"] = 8;
	sBlendShapeStringMap["eyeBlinkRight"] = 9;
	sBlendShapeStringMap["eyeLookDownLeft"] = 10;
	sBlendShapeStringMap["eyeLookDownRight"] = 11;
	sBlendShapeStringMap["eyeLookInLeft"] = 12;
	sBlendShapeStringMap["eyeLookInRight"] = 13;
	sBlendShapeStringMap["eyeLookOutLeft"] = 14;
	sBlendShapeStringMap["eyeLookOutRight"] = 15;
	sBlendShapeStringMap["eyeLookUpLeft"] = 16;
	sBlendShapeStringMap["eyeLookUpRight"] = 17;
	sBlendShapeStringMap["eyeSquintLeft"] = 18;
	sBlendShapeStringMap["eyeSquintRight"] = 19;
	sBlendShapeStringMap["eyeWideLeft"] = 20;
	sBlendShapeStringMap["eyeWideRight"] = 21;
	sBlendShapeStringMap["jawForward"] = 22;
	sBlendShapeStringMap["jawLeft"] = 23;
	sBlendShapeStringMap["jawOpen"] = 24;
	sBlendShapeStringMap["jawRight"] = 25;
	sBlendShapeStringMap["mouthClose"] = 26;
	sBlendShapeStringMap["mouthDimpleLeft"] = 27;
	sBlendShapeStringMap["mouthDimpleRight"] = 28;
	sBlendShapeStringMap["mouthFrownLeft"] = 29;
	sBlendShapeStringMap["mouthFrownRight"] = 30;
	sBlendShapeStringMap["mouthFunnel"] = 31;
	sBlendShapeStringMap["mouthLeft"] = 32;
	sBlendShapeStringMap["mouthLowerDownLeft"] = 33;
	sBlendShapeStringMap["mouthLowerDownRight"] = 34;
	sBlendShapeStringMap["mouthPressLeft"] = 35;
	sBlendShapeStringMap["mouthPressRight"] = 36;
	sBlendShapeStringMap["mouthPucker"] = 37;
	sBlendShapeStringMap["mouthRight"] = 38;
	sBlendShapeStringMap["mouthRollLower"] = 39;
	sBlendShapeStringMap["mouthRollUpper"] = 40;
	sBlendShapeStringMap["mouthShrugLower"] = 41;
	sBlendShapeStringMap["mouthShrugUpper"] = 42;
	sBlendShapeStringMap["mouthSmileLeft"] = 43;
	sBlendShapeStringMap["mouthSmileRight"] = 44;
	sBlendShapeStringMap["mouthStretchLeft"] = 45;
	sBlendShapeStringMap["mouthStretchRight"] = 46;
	sBlendShapeStringMap["mouthUpperUpLeft"] = 47;
	sBlendShapeStringMap["mouthUpperUpRight"] = 48;
	sBlendShapeStringMap["noseSneerLeft"] = 49;
	sBlendShapeStringMap["noseSneerRight"] = 50;
	sBlendShapeStringMap["tongueOut"] = 51;
	sBlendShapeStringMap["unknown"] = 52;
	
	// this is off by 1
	for( auto iter = sBlendShapeStringMap.begin(); iter != sBlendShapeStringMap.end(); iter++ ) {
		iter->second += 1;
	}
}

//-------------------------------------------
Face::BlendShapeType Face::sGetBlendShape( const std::string& abtype ) {
	if(sBlendShapeStringMap.size() < 1 ) {
		sCreateBlendShapeStringMap();
	}
	
	if( sBlendShapeStringMap.count(abtype) > 0 ) {
		return (BlendShapeType)sBlendShapeStringMap[abtype];
	}
	return UNKNOWN;
}

//-------------------------------------------
std::string Face::sGetBlendShapeAsString( BlendShapeType abtype ) {
	if(sBlendShapeStringMap.size() < 1 ) {
		sCreateBlendShapeStringMap();
	}
	for( auto iter = sBlendShapeStringMap.begin(); iter != sBlendShapeStringMap.end(); iter++ ) {
		if( iter->second == (int)abtype ) {
			return iter->first;
		}
	}
	return "unknown";
}

////-------------------------------------------
//bool Face::shouldRemove( const std::shared_ptr<Face>& aface ) {
//	return (!aface || aface->bRemove);
//}

//-------------------------------------------
bool Face::sortBlendShapesOnIndex( const BlendShape& aa, const BlendShape& ab ) {
	return aa.index < ab.index;
}

//-------------------------------------------
bool Face::sortBlendShapesOnScore( const BlendShape& aa, const BlendShape& ab ) {
	return aa.score > ab.score;
}

//-------------------------------------------
void Face::updateFrom(std::shared_ptr<Face> &aother) {
	keypoints = aother->keypoints;
	updateFromKeypoints();
	updateBlendShapes(aother->getIncomingBlendShapes());
}

//-------------------------------------------
void Face::updateFromFaceWithSmoothing( std::shared_ptr<Face>& aother, float pct ) {
	updateKeypointsFromOtherWithSmoothing( aother, pct );
	updateBlendShapes(aother->getIncomingBlendShapes());
}

//-------------------------------------------
void Face::updateFromKeypoints() {
	updateDrawMeshes();
	_updateAxes();
	
	mKeypointsLineMesh.clearIndices();
	if( keypoints.size() > 1 ) {
//		mLinesMesh.setMode( OF_PRIMITIVE_LINES );
		
//		mLinesMesh.clear();
		// now lets set the colors and vertices number //
//		mLinesMesh.getVertices().assign(vertices.size(), glm::vec3(0.f, 0.f, 0.f));
		if( mKeypointsLineMesh.getNumColors() != mKeypointsLineMesh.getNumVertices() ) {
			mKeypointsLineMesh.getColors().assign( mKeypointsLineMesh.getNumVertices(), ofColor::grey );
		}
		
		// add eyebrows //
		_addToLinesMesh( mKeypointsLineMesh, getEyeIndices(false), ofColor::green );
		_addToLinesMesh( mKeypointsLineMesh, getEyeIrisIndices(false), ofColor::green );
		_addToLinesMesh( mKeypointsLineMesh, getEyebrowIndices(false), ofColor::green );
		
		_addToLinesMesh( mKeypointsLineMesh, getEyeIndices(true), ofColor::blue );
		_addToLinesMesh( mKeypointsLineMesh, getEyeIrisIndices(true), ofColor::blue );
		_addToLinesMesh( mKeypointsLineMesh, getEyebrowIndices(true), ofColor::blue );
		
		_addToLinesMesh( mKeypointsLineMesh, getOuterMouthIndices(), ofColor::cyan );
		_addToLinesMesh( mKeypointsLineMesh, getInnerMouthIndices(), ofColor::darkCyan );
		
		_addToLinesMesh( mKeypointsLineMesh, getOutlineIndices(), ofColor::white );
		
//		mLinesMesh2d = mLinesMesh;
	}
}

//-------------------------------------------
void Face::updateBlendShapes(const std::vector< Face::BlendShape >& aOtherBlendShapes) {
//	mIncomingBlendShapes = aOtherBlendShapes;
	if( aOtherBlendShapes.size() < 1 ) {
		return;
	}
	if( mBlendShapes.size() != aOtherBlendShapes.size() ) {
		//std::cout << "mBlendShapes.size() != aOtherBlendShapes.size()!!!!!! " << std::endl;
		mBlendShapes = aOtherBlendShapes;
	}
	
	size_t numOtherShapes = aOtherBlendShapes.size();
	for( size_t i = 0; i < numOtherShapes; i++ ) {
		mBlendShapes[i].index = aOtherBlendShapes[i].index;
		mBlendShapes[i].category_name = aOtherBlendShapes[i].category_name;
		mBlendShapes[i].type = aOtherBlendShapes[i].type;
		mBlendShapes[i].score = aOtherBlendShapes[i].score;
//		std::cout << i << " bshape: " << mBlendShapes[i].index << " cat name: " << mBlendShapes[i].category_name << " aOtherBlendShapes[i].cat: " << aOtherBlendShapes[i].category_name << std::endl;
	}
}

//-------------------------------------------
void Face::setIncomingBlendShape( const std::string& aCatName, const float& ascore, const int& aindex ) {
	
	if(mIncomingBlendShapes.size() < 1 ) {
		mIncomingBlendShapes.assign(BlendShapeType::TOTAL, BlendShape() );
		
		// not recognizing tongue out for some reason
		mIncomingBlendShapes[TONGUE_OUT].category_name = "tongueOut";
		mIncomingBlendShapes[TONGUE_OUT].type = sGetBlendShape(mIncomingBlendShapes[TONGUE_OUT].category_name);
		mIncomingBlendShapes[TONGUE_OUT].index = (int)TONGUE_OUT;
	}
 
	if( aindex >= mIncomingBlendShapes.size() && aindex < 100) {
		std::cout << "setIncomingBlendShape :: ALLOCATING MORE INCOMING BLEND SHAPES " << aindex << " mIncomingBlendShapes: " << mIncomingBlendShapes.size() << std::endl;
//		mIncomingBlendShapes.assign(aindex+1, BlendShape() );
		int numToAdd = (aindex+1)-mIncomingBlendShapes.size();
		for( int i = 0; i < numToAdd; i++ ) {
			mIncomingBlendShapes.push_back(BlendShape());
		}
	}
	
	BlendShape bshape;
	bshape.index = aindex;
	bshape.type = sGetBlendShape(aCatName);
	bshape.category_name = aCatName;
	bshape.score = ascore;
	
//	std::cout << "setIncomingBlendShape " << aindex << " - aCatName: " << aCatName <<  "  bshape.cat name: " << bshape.category_name << " score: " << ascore << std::endl;
	
	mIncomingBlendShapes[aindex] = bshape;
}

//-------------------------------------------
std::vector< Face::BlendShape >& Face::getIncomingBlendShapes() {
	return mIncomingBlendShapes;
}

//-------------------------------------------
std::vector< Face::BlendShape >& Face::getBlendShapes() {
	return mBlendShapes;
}

//-------------------------------------------
Face::BlendShape& Face::getBlendShape( const std::string& acategory ) {
	for( auto& bshape : mBlendShapes ) {
		if( bshape.category_name == acategory ) {
			return bshape;
		}
	}
	dummyShape.category_name = "unknown";
	dummyShape.type = UNKNOWN;
	return dummyShape;
}

//-------------------------------------------
Face::BlendShape& Face::getBlendShape( const BlendShapeType& atype ) {
	for( auto& bshape : mBlendShapes ) {
		if( bshape.type == atype ) {
			return bshape;
		}
	}
	dummyShape.category_name = "unknown";
	dummyShape.type = UNKNOWN;
	return dummyShape;
}

//-------------------------------------------
std::vector<Face::BlendShape> Face::getTopBlendShapes( int aTopToReturn ) {
	std::vector<Face::BlendShape> sortedShapes;
	if( mBlendShapes.size() < aTopToReturn || aTopToReturn < 1) {
		sortedShapes = mBlendShapes;
	} else {
		sortedShapes = std::vector<Face::BlendShape>( mBlendShapes.begin(), mBlendShapes.begin()+aTopToReturn );
	}
	ofSort(sortedShapes, sortBlendShapesOnScore );
	return sortedShapes;
}

//-------------------------------------------
void Face::drawIrises( float aRadius, bool aBUseZ ) {
	if( mVertices.size() < 2 ) {
		return;
	}
	auto rindex = getIrisCenterIndex(true);
	auto lindex = getIrisCenterIndex(false);
	glm::vec3 rpos = mVertices[rindex];
	glm::vec3 lpos = mVertices[lindex];
	if( !aBUseZ ) {
		rpos.z = 0.f;//vertices2d[rindex];
		lpos.z = 0.f;//vertices2d[lindex];
	}
	ofDrawCircle( rpos, aRadius );
	ofDrawCircle( lpos, aRadius );
}

//-------------------------------------------
std::vector<ofIndexType> Face::getEyebrowIndices(bool bRight) {
	if( bRight ) {
		return {
			107, 66, 105, 63, 70,
			46, 53, 52, 65, 55
		};
	}
	return {
		336, 296, 334, 293, 300,
		276, 283, 282, 295, 285
	};
}

//-------------------------------------------
std::vector<ofIndexType> Face::getEyebrowIndicesTop(bool bRight) {
	if( bRight ) {
		return { 107, 66, 105, 63, 70 };
	}
	return { 336, 296, 334, 293, 300 };
}

//-------------------------------------------
std::vector<ofIndexType> Face::getEyebrowIndicesBottom(bool bRight) {
	if( bRight ) {
		return { 46, 53, 52, 65, 55 };
	}
	return { 276, 283, 282, 295, 285 };
}

//-------------------------------------------
std::vector<ofIndexType> Face::getEyeIndices(bool bRight) {
	if( bRight ) {
		return {
			133, 173, 157, 158, 159, 160, 161, 246,
			33, 7, 163, 144, 145, 153, 154, 155
		};
	}
	return {
		362, 398, 384, 385, 386, 387, 388, 466,
		263, 249, 390, 373, 374, 380, 381, 382
	};
}

// includes both corner points
//-------------------------------------------
std::vector<ofIndexType> Face::getEyeIndicesTop(bool bRight) {
	if( bRight ) {
		return { 133, 173, 157, 158, 159, 160, 161, 246, 33};
	}
	return {362, 398, 384, 385, 386, 387, 388, 466, 263};
}

//-------------------------------------------
std::vector<ofIndexType> Face::getEyeIndicesBottom(bool bRight) {
	if( bRight ) {
		return { 33, 7, 163, 144, 145, 153, 154, 155, 133};
	}
	return {263, 249, 390, 373, 374, 380, 381, 382, 362};
}

//-------------------------------------------
std::vector<ofIndexType> Face::getEyeIrisIndices(bool bRight) {
	if( bRight ) {
		return { 470, 471, 472, 469};
	}
	return { 475, 476, 477, 474};
}

//-------------------------------------------
std::vector<ofIndexType> Face::getOuterMouthIndices() {
	return {
		61, 185, 40, 39, 37, 0, 267, 269, 270, 409, 291,
		375, 321, 405, 314, 17, 84, 181, 91, 146,
	};
}

//-------------------------------------------
std::vector<ofIndexType> Face::getInnerMouthIndices() {
	return {
		78, 191, 80, 81, 82, 13, 312, 311, 310, 415, 308,
		324, 318, 402, 317, 14, 87, 178, 88, 95,
	};
}

//-------------------------------------------
std::vector<ofIndexType> Face::getInnerMouthIndicesTop() {
	return {
		78, 191, 80, 81, 82, 13, 312, 311, 310, 415, 308
	};
}

//-------------------------------------------
std::vector<ofIndexType> Face::getInnerMouthIndicesBottom() {
	return {
		308, 324, 318, 402, 317, 14, 87, 178, 88, 95, 78
	};
}

//-------------------------------------------
std::vector<ofIndexType> Face::getOutlineIndices() {
	return {
		10, 338, 297, 332, 284, 251, 389, 356, 454, 323, 361, 288, 397, 365, 379, 378, 400, 377, 152,
		148, 176, 149, 150, 136, 172, 58, 132, 93, 234, 127, 162, 21, 54, 103, 67, 109
	};
}

//-------------------------------------------
glm::vec3 Face::getPosition() {
	auto rpos =  (keypoints[KP_EAR_LEFT].pos + keypoints[KP_EAR_RIGHT].pos) * 0.5f;
	return rpos;
}

//-------------------------------------------
glm::vec3 Face::getPositionWorld() {
	auto rpos =  (keypoints[KP_EAR_LEFT].posWorld + keypoints[KP_EAR_RIGHT].posWorld) * 0.5f;
	return rpos;
}

//-------------------------------------------
glm::vec3 Face::getPositionNormalized() {
	auto rpos =  (keypoints[KP_EAR_LEFT].posN + keypoints[KP_EAR_RIGHT].posN) * 0.5f;
	return rpos;
}

//-------------------------------------------
ofIndexType Face::getIrisCenterIndex( bool bRight ) {
	if( bRight ) {
		return KP_EYE_IRIS_RIGHT;
	}
	return KP_EYE_IRIS_LEFT;
}

//-------------------------------------------
glm::vec3& Face::getRightIrisPos() {
	return getPosForIndex(getIrisCenterIndex(true));
}

//-------------------------------------------
glm::vec3& Face::getLeftIrisPos() {
	return getPosForIndex(getIrisCenterIndex(false));
}

////-------------------------------------------
//glm::vec3& Face::getPosForIndex( const ofIndexType& aindex ) {
//	if( mVertices.size() < 2 ) {
//		return dummyPos;
//	}
//	return mVertices[aindex];
//}

//-------------------------------------------
bool Face::isRightEyeBlinking() {
	auto& bshape = getBlendShape(EYE_BLINK_RIGHT);
//	return bshape.score > 0.5f && bshape.timeActive > 0.1f;
	return bshape.bChangedToActive;
}

//-------------------------------------------
bool Face::isLeftEyeBlinking() {
	auto& bshape = getBlendShape(EYE_BLINK_LEFT);
//	return bshape.score > 0.5f && bshape.timeActive > 0.1f;
	return bshape.bChangedToActive;
}

//-------------------------------------------
float Face::getEyeOpenPercent(bool bRight) {
	auto topMidPt = getPosForIndex(KP_EYE_RIGHT_MID_TOP);
	auto botMidPt = getPosForIndex(KP_EYE_RIGHT_MID_BOTTOM);
	auto leftPt = getPosForIndex(KP_EYE_RIGHT_INNER);
	auto rightPt = getPosForIndex(KP_EYE_RIGHT_OUTER);
	
	if( !bRight ) {
		topMidPt = getPosForIndex(KP_EYE_LEFT_MID_TOP);
		botMidPt = getPosForIndex(KP_EYE_LEFT_MID_BOTTOM);
		leftPt = getPosForIndex(KP_EYE_LEFT_INNER);
		rightPt = getPosForIndex(KP_EYE_LEFT_OUTER);
	}
	
	float theight = glm::length(topMidPt - botMidPt);
	float twidth =  std::max(0.01f, glm::length(rightPt - leftPt));
	
	return ofMap( theight / twidth, 0.15f, 0.6f, 0.0f, 1.0f, true );
}

//-------------------------------------------
float Face::getMouthOpenPercent() {
	auto topMidPt = getPosForIndex(KP_MOUTH_INNER_MID_TOP);
	auto botMidPt = getPosForIndex(KP_MOUTH_INNER_MID_BOTTOM);
	auto leftPt = getPosForIndex(KP_MOUTH_INNER_LEFT);
	auto rightPt = getPosForIndex(KP_MOUTH_INNER_RIGHT);
	
	float theight = glm::length(topMidPt - botMidPt);
	float twidth =  std::max(0.01f, glm::length(rightPt - leftPt));
	
	return ofMap( theight / twidth, 0.05f, 0.8f, 0.0f, 1.0f, true );
}

//-------------------------------------------
float Face::getEyebrowRaisePercent(bool bRight) {
	auto topMidPt = getPosForIndex(52);
	auto leftPt = getPosForIndex(KP_EYEBROW_RIGHT_OUTER_BOTTOM);
	auto rightPt = getPosForIndex(KP_EYEBROW_RIGHT_INNER_BOTTOM);
	
	if(!bRight) {
		topMidPt = getPosForIndex(282);
		leftPt = getPosForIndex(KP_EYEBROW_LEFT_OUTER_BOTTOM);
		rightPt = getPosForIndex(KP_EYEBROW_LEFT_INNER_BOTTOM);
	}
	
	// this method doesn't seem very robust
	
	auto botMidPt = (leftPt+rightPt) * 0.5f;
	
	float theight = glm::length(topMidPt - botMidPt);
	float twidth =  std::max(0.01f, glm::length(rightPt - leftPt));
	
//	ofLogNotice("Face :: eyebrow raise : ") << "right: " << bRight << " pct: " << (theight / twidth) << " | " << ofGetFrameNum();
	
	return ofMap( theight / twidth, 0.2f, 0.25f, 0.0f, 1.0f, true );
}

//-------------------------------------------
void Face::_addToLinesMesh( ofMesh& amesh, const std::vector<ofIndexType>& aindices, ofFloatColor acolor ) {
//	std::vector<ofFloatColor> colors;
//	colors.assign(aindices.size(), acolor );
	
	size_t numIndices = aindices.size();
	for( size_t i = 0; i < numIndices; i++ ) {
		amesh.getColors()[aindices[i] ] = acolor;
		amesh.addIndex( aindices[i] );
		if( i == numIndices-1 ) {
			amesh.addIndex( aindices[0] );
		} else {
			amesh.addIndex( aindices[i + 1] );
		}
	}
}

// maybe try this
// http://glm.g-truc.net/0.9.3/api/a00199.html#ga4da5fddb2f1aa679fdb45abd344efffb
//-------------------------------------------
void Face::_updateAxes() {
	if( keypoints.size() < 10 ) return;
	// figure out the color angle //
	auto earRight = getPosForIndex( ofx::MediaPipe::Face::KP_EAR_RIGHT );
	auto earLeft = getPosForIndex( ofx::MediaPipe::Face::KP_EAR_LEFT );
	
	auto forehead = getPosForIndex( ofx::MediaPipe::Face::KP_FOREHEAD );
	auto chin = getPosForIndex( ofx::MediaPipe::Face::KP_CHIN );
	
	auto tPos = (earRight + earLeft + forehead + chin) / 4.f;
	
	
	auto sideDir = Utils::normalize(earLeft-earRight, glm::vec3(1.f, 0.f, 0.f));
	auto upDir = Utils::normalize(forehead-chin, glm::vec3(0.f, 1.f, 0.0f));
	auto forward = Utils::normalize(glm::cross(sideDir, upDir), glm::vec3(0.f, 0.f, 1.f));
	
	// Recompute side to ensure it's orthogonal to both up and forward
//	sideDir = glm::normalize(glm::cross(forward,upDir));
	
	if( !Utils::isValid(sideDir) || !Utils::isValid(upDir) || !Utils::isValid(forward) ) {
		sideDir = glm::vec3(1.f, 0.0f, 0.0f);
		upDir = glm::vec3( 0.0f, 1.f, 0.0f);
		forward = glm::vec3(0.0f, 0.0f, 1.f);
	}
	
	
	mFaceDirs[0] = sideDir;
	mFaceDirs[1] = upDir;
	mFaceDirs[2] = forward;
	
	//		auto orientation = glm::orientation(upDir, sideDir);
	glm::mat3 m;
	m[0] = sideDir;
	m[1] = upDir;
	m[2] = forward;
	
	mOrientation = (glm::toQuat(m));
	
	
	glm::vec3 sideNorm = Utils::normalize(earLeft-earRight, glm::vec3(1.f, 0.0f, 0.0f));
	sideNorm.z *= -1.f;
	glm::vec3 upNorm = Utils::normalize(forehead-chin, glm::vec3(0.f, 1.f, 0.f));
	upNorm.z *= -1.f;
	
	glm::vec3 projection = glm::dot(upNorm, sideNorm) * sideNorm;
	upNorm = Utils::normalize(upNorm - projection, glm::vec3(0.f, 1.f, 0.f));
	
	// Project sideNorm onto upNorm
//	glm::vec3 projection = glm::dot(sideNorm, upNorm) * upNorm;
	
	// Subtract the projection from sideNorm to make it perpendicular to upNorm
//	sideNorm = glm::normalize(sideNorm - projection);
	
	forward = Utils::normalize(glm::cross(upNorm, sideNorm), glm::vec3(0.f, 0.f, 1.f));
	
	projection = glm::dot(forward, upNorm) * upNorm;
	forward = Utils::normalize(forward - projection, glm::vec3(0.f, 0.f, 1.f));
	
	// Recompute side to ensure it's orthogonal to both up and forward
	sideNorm = glm::normalize(glm::cross(forward,upNorm));
	
	if( !Utils::isValid(sideNorm) || !Utils::isValid(upNorm) || !Utils::isValid(forward) ) {
		sideNorm = glm::vec3(1.f, 0.0f, 0.0f);
		upNorm = glm::vec3( 0.0f, -1.f, 0.0f);
		forward = glm::vec3(0.0f, 0.0f, -1.f);
	}
	
	mFaceScreenDirs[0] = sideNorm;
	mFaceScreenDirs[1] = upNorm;
	mFaceScreenDirs[2] = forward;
	
	// Create a rotation matrix from the three vectors
	glm::mat3 rotationMatrix(
							 mFaceScreenDirs[0],     // X-axis (side vector)
							 mFaceScreenDirs[1],       // Y-axis (up vector)
							 -mFaceScreenDirs[2]       // Z-axis (forward vector)
							 );
	
	
	auto rotQ = glm::quat_cast(rotationMatrix);
//	glm::quat rightHandedQuat(rotQ.w, -rotQ.x, (rotQ.y), -rotQ.z);
	
	mScreenOrientation = rotQ;//(glm::toQuat(m));
//	mScreenOrientation = glm::conjugate(mOrientation);//(glm::toQuat(m));
	
//	mScreenOrientation = glm::angleAxis(glm::radians(180.f), glm::vec3(0.f, 1.f, 0.0f)) * mScreenOrientation;//(glm::toQuat(m));
	
}
