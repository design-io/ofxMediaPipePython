//
//  ofxMediaPipeFrame.cpp
//  ShowMeFace
//
//  Created by Nick Hardeman on 5/17/24.
//

#include "ofxMediaPipeFrame.h"

using std::string;
using std::shared_ptr;
using std::vector;
using std::make_shared;
using std::cout;
using std::endl;

using namespace ofx::MediaPipe;

//--------------------------------------------------------------
bool Frame::setup( ofJson& aJFrame) {
	mFaces.clear();
	mPoses.clear();
	mHands.clear();
	
	timestamp = aJFrame["timestamp"];
	std::string stype = aJFrame["type"];
	mType = TrackedObject::sGetTypeFromString(stype);
	int num = aJFrame["num"];
	auto& jobjects = aJFrame["objects"];
	if( jobjects.empty() ) {
		return false;
	}
	
	for( auto& jobj : jobjects ) {
		std::shared_ptr<TrackedObject> tobj;
		if( mType == TrackedObject::FACE ) {
			auto face = make_shared<Face>();
			
			auto& jblendShapes = jobj["blendShapes"];
			if( !jblendShapes.empty() ) {
				for( auto& jblendShape : jblendShapes ) {
					face->setIncomingBlendShape(jblendShape["category_name"], jblendShape["score"], jblendShape["index"] );
				}
			}
			
//			if( mSettings.outputFaceBlendshapes ) {
//				py::list face_blendshapes = face_blendshapes_list[i];
//				int numBlends = py::len(face_blendshapes);
//				for( int j = 0; j < numBlends; j++ ) {
//					py::object hbs = face_blendshapes[j];
//					std::string hcategory = py::str(hbs.attr("category_name"));
//					float hscore = py::float_(hbs.attr("score"));
//					int hindex = py::int_(hbs.attr("index"));
//					//				std::cout << "index: " << hindex << " hcategory: " << hcategory << " score: " << hscore << std::endl;
//					tface->setIncomingBlendShape(hcategory, hscore, hindex);
//				}
//			}
			
			mFaces.push_back(face);
			tobj = face;
			
		} else if( mType == TrackedObject::HAND ) {
			auto hand = make_shared<Hand>();
			
			hand->index = jobj["index"];
			if( jobj["handed"] == "left" ) {
				hand->handed = Hand::Handedness::LEFT;
			}
//			jhanded["index"] = hand->index;
//			jhanded["handed"] = (hand->handed == Hand::Handedness::LEFT) ? "left" : "right";
			
			mHands.push_back(hand);
			tobj = hand;
		} else if( mType == TrackedObject::POSE ) {
			auto pose = make_shared<Pose>();
			
			mPoses.push_back(pose);
			tobj = pose;
		} else {
			ofLogWarning("ofx::MediaPipe::Frame::setup") << " unable to process: " << stype;
			continue;
		}
		
		if( tobj ) {
			if( jobj.count("ID") > 0 ) {
				tobj->ID = jobj["ID"];
			}
			auto& jkps = jobj["kps"];
			for( auto& jkp : jkps ) {
				TrackedObject::Keypoint kp;
				kp.posN.x = jkp["xN"];
				kp.posN.y = jkp["yN"];
				kp.posN.z = jkp["zN"];
				
//				kp.pos = kp.posN;
//				kp.pos.x *= aOutRect.width;
//				kp.pos.y *= aOutRect.height;
//				kp.pos.x += aOutRect.x;
//				kp.pos.y += aOutRect.y;
//				
//				kp.pos.z *= aOutRect.width;
				
				
				kp.posWorld.x = jkp["xW"];
				kp.posWorld.y = jkp["yW"];
				kp.posWorld.z = jkp["zW"];
				
				tobj->keypoints.push_back(kp);
			}
		}
	}
	return true;
}

//--------------------------------------------------------------
ofJson Frame::jsonify( const std::int64_t& atimestamp, const std::vector<std::shared_ptr<Face>>& afaces ) {
	ofJson jfacesHeader;
	jfacesHeader["type"] = TrackedObject::sGetTypeAsString( TrackedObject::FACE );
	jfacesHeader["timestamp"] = atimestamp;
	jfacesHeader["num"] = afaces.size();
	
	ofJson jfaces;
	
	for( const auto& face : afaces ) {
		ofJson jface;
		_serialize(jface, face);
		// add blendshapes //
		ofJson jBlendies;
		auto& bshapes = face->getBlendShapes();
		for( auto& bshape : bshapes ) {
			ofJson jbShape;
			jbShape["category_name"] = bshape.category_name;
			jbShape["score"] = bshape.score;
			jbShape["index"] = bshape.index;
			jBlendies.push_back(jbShape);
		}
		
		jface["blendShapes"] = jBlendies;
		jfaces.push_back(jface);
	}
	
	jfacesHeader["objects"] = jfaces;
	
	return jfacesHeader;
}

//--------------------------------------------------------------
ofJson Frame::jsonify( const std::int64_t& atimestamp, const std::vector<std::shared_ptr<Hand>>& ahands ) {
	ofJson jhandsHeader;
	jhandsHeader["type"] = TrackedObject::sGetTypeAsString( TrackedObject::HAND );
	jhandsHeader["timestamp"] = atimestamp;
	jhandsHeader["num"] = ahands.size();
	
	ofJson jhands;
	for( const auto& hand : ahands ) {
		ofJson jhand;
		_serialize(jhand, hand);
		jhand["index"] = hand->index;
		jhand["handed"] = (hand->handed == Hand::Handedness::LEFT) ? "left" : "right";
		jhands.push_back(jhand);
	}
	jhandsHeader["objects"] = jhands;
	return jhandsHeader;
}

//--------------------------------------------------------------
ofJson Frame::jsonify( const std::int64_t& atimestamp, const std::vector<std::shared_ptr<Pose>>& aposes ) {
	ofJson jposesHeader;
	
	jposesHeader["type"] = TrackedObject::sGetTypeAsString( TrackedObject::POSE );
	jposesHeader["timestamp"] = atimestamp;
	jposesHeader["num"] = aposes.size();
	
	ofJson jposes;
	for( const auto& pose : aposes ) {
		ofJson jpose;
		_serialize(jpose,pose);
		jposes.push_back(jpose);
		// segmentation masks, I don't think so
	}
	jposesHeader["objects"] = jposes;
	return jposesHeader;
}

#if defined(OF_ADDON_HAS_OFX_OSC)
//--------------------------------------------------------------
ofxOscMessage Frame::getOscMessage( const std::uint64_t& aFrameNum, std::shared_ptr<Face> aface ) {
	ofxOscMessage m;
	m.setAddress("/ofxmp/faces");
	_setupOscMessage( m, aFrameNum, aface, false );
	return m;
}

//--------------------------------------------------------------
ofxOscMessage Frame::getOscMessage( const std::uint64_t& aFrameNum, std::shared_ptr<Hand> ahand ) {
	ofxOscMessage m;
	m.setAddress("/ofxmp/hands");
	_setupOscMessage( m, aFrameNum, ahand, false );
	return m;
}

//--------------------------------------------------------------
ofxOscMessage Frame::getOscMessage( const std::uint64_t& aFrameNum, std::shared_ptr<Pose> apose ) {
	ofxOscMessage m;
	m.setAddress("/ofxmp/poses");
	_setupOscMessage( m, aFrameNum, apose, false );
	return m;
}

//--------------------------------------------------------------
ofxOscMessage Frame::getOscMessageWorld( const std::uint64_t& aFrameNum, std::shared_ptr<Face> aface ) {
	ofxOscMessage m;
	m.setAddress("/ofxmp/facesW");
	_setupOscMessage( m, aFrameNum, aface, true );
	return m;
}

//--------------------------------------------------------------
ofxOscMessage Frame::getOscMessageWorld( const std::uint64_t& aFrameNum, std::shared_ptr<Hand> ahand ) {
	ofxOscMessage m;
	m.setAddress("/ofxmp/handsW");
	_setupOscMessage( m, aFrameNum, ahand, true );
	return m;
}

//--------------------------------------------------------------
ofxOscMessage Frame::getOscMessageWorld( const std::uint64_t& aFrameNum, std::shared_ptr<Pose> apose ) {
	ofxOscMessage m;
	m.setAddress("/ofxmp/posesW");
	_setupOscMessage( m, aFrameNum, apose, true );
	return m;
}

//--------------------------------------------------------------
bool Frame::setup( ofxOscMessage& am, std::shared_ptr<TrackedObject>& aobject, bool abWorld) {
	if( am.getNumArgs() < 2 ) return false;
	
	if( !aobject ) return false;
	
	std::int64_t frame = am.getArgAsInt64(0);
	std::int32_t tid = am.getArgAsInt32(1);
	
	auto numArgs = am.getNumArgs();
	
	auto numKPS = (numArgs-2)/3;
	if( aobject->keypoints.size() != numKPS ) {
		ofLogVerbose("ofx::MediaPipe::Frame") << "resizing keypoints to " << numKPS;
		aobject->keypoints.resize(numKPS);
	}
	
	int kpIndex = 0;
	for( std::size_t sindex = 2; sindex < numArgs; sindex += 3 ) {
		float fx = am.getArgAsFloat(sindex+0);
		float fy = am.getArgAsFloat(sindex+1);
		float fz = am.getArgAsFloat(sindex+2);
		
		if( abWorld ) {
			aobject->keypoints[kpIndex].posWorld = glm::vec3(fx, fy, fz);
		} else {
			aobject->keypoints[kpIndex].posN = glm::vec3(fx, fy, fz);
		}
		kpIndex++;
		if( kpIndex >= numKPS ) {
			break;
		}
	}
	
	
	return true;
}

#endif

//--------------------------------------------------------------
const double Frame::getTimestampSeconds() {
	std::chrono::nanoseconds ns_duration(timestamp);
	std::chrono::duration<double> seconds_duration = ns_duration;
	return seconds_duration.count();
	
	// below also works 
	//return (double)timestamp / 1000000000.f;
}

//--------------------------------------------------------------
void Frame::_serialize(ofJson& ajobj, const std::shared_ptr<TrackedObject>& aobject ) {
//	ofJson jobject;
//	ajobj["timestamp"] = timestamp;
	ajobj["ID"] = aobject->ID;
	ajobj["type"] = TrackedObject::sGetTypeAsString( aobject->getType() );
	ajobj["kps"] = _getJsonFromKeypoints(aobject);
//	return ajobj;
}

//--------------------------------------------------------------
ofJson Frame::_getJsonFromKeypoints( const std::shared_ptr<TrackedObject>& aobject ) {
//	glm::vec3 pos = {0.f, 0.f, 0.f};
//	glm::vec3 posN = {0.f, 0.f, 0.f};  normalized position
//	glm::vec3 posWorld = {0.f, 0.f, 0.f};
	ofJson jkeyObj;
	for(size_t i = 0; i < aobject->keypoints.size(); i++ ) {
		auto& kp = aobject->keypoints[i];
		ofJson jkp;
		jkp["i"] = i;
//		jkp["posx"] = kp.pos.x;
//		jkp["posy"] = kp.pos.y;
//		jkp["posz"] = kp.pos.z;
		
		jkp["xN"] = kp.posN.x;
		jkp["yN"] = kp.posN.y;
		jkp["zN"] = kp.posN.z;
		
		jkp["xW"] = kp.posWorld.x;
		jkp["yW"] = kp.posWorld.y;
		jkp["zW"] = kp.posWorld.z;
		jkeyObj.push_back(jkp);
	}
	return jkeyObj;
}

#if defined(OF_ADDON_HAS_OFX_OSC)
//--------------------------------------------------------------
bool Frame::_setupOscMessage( ofxOscMessage& am, const std::uint64_t& aFrameNum, const std::shared_ptr<TrackedObject>& aobject, bool abWorld ) {
	
	am.addInt64Arg( (std::int64_t) aFrameNum );
	am.addInt32Arg( (std::int32_t)aobject->ID );
	
	if( aobject->keypoints.size() > 0 ) {
		for(size_t i = 0; i < aobject->keypoints.size(); i++ ) {
			auto& kp = aobject->keypoints[i];
			if( abWorld ) {
				am.addFloatArg(kp.posWorld.x);
				am.addFloatArg(kp.posWorld.y);
				am.addFloatArg(kp.posWorld.z);
			} else {
				am.addFloatArg(kp.posN.x);
				am.addFloatArg(kp.posN.y);
				am.addFloatArg(kp.posN.z);
			}
		}
		return true;
	}
	return false;
}
#endif

