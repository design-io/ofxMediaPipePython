//
//  ofxMediaPipeUtils.h
//  HandControls
//
//  Created by Nick Hardeman on 4/24/24.
//

#pragma once
#if !defined(OFX_MEDIAPIPE_EXCLUDE_TRACKERS)
#include <pybind11/embed.h>
namespace py = pybind11;
#endif
#include "ofVectorMath.h"

namespace ofx::MediaPipe {
class Utils {
public:
	
#if !defined(OFX_MEDIAPIPE_EXCLUDE_TRACKERS)
	static bool has_attribute(py::object& obj, const std::string &attr_name);
	static bool has_all_attributes(py::object& obj, const std::vector<std::string>& attr_names);
	
	static void print_attributes(py::object& aobject);
#endif
	
	//--------------------------------------------------------------
	static bool isValid( const glm::vec3& av ) {
		if( glm::isnan(av.x) || glm::isnan(av.y) || glm::isnan(av.z) ) {
			return false;
		}
		if( glm::isinf(av.x) || glm::isinf(av.y) || glm::isinf(av.z) ) {
			return false;
		}
		return true;
	}
	
	//--------------------------------------------------------------
	static bool isValid( const glm::quat& aq ) {
		if( glm::isnan(aq.x) ||  glm::isnan(aq.y) ||  glm::isnan(aq.z) ||  glm::isnan(aq.w) ) {
			return false;
		}
		if( glm::isinf(aq.x) ||  glm::isinf(aq.y) ||  glm::isinf(aq.z) ||  glm::isinf(aq.w) ) {
			return false;
		}
		return true;
	}
	
	static glm::vec3 normalize( const glm::vec3& av ) {
		if( glm::length2(av) < glm::epsilon<float>() ) {
			return glm::vec3(0.f, 0.f, 0.f );
		}
		return glm::normalize(av);
	}
	
	static glm::vec3 normalize( const glm::vec3& av, const glm::vec3& adefault ) {
		if( glm::length2(av) < glm::epsilon<float>() ) {
			return adefault;
		}
		return glm::normalize(av);
	}
	
};
}
