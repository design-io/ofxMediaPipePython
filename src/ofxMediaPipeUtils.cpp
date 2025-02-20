//
//  ofxMediaPipeUtils.cpp
//  HandControls
//
//  Created by Nick Hardeman on 4/24/24.
//

#include "ofxMediaPipeUtils.h"
#include "ofLog.h"

using namespace ofx::MediaPipe;

#if !defined(OFX_MEDIAPIPE_EXCLUDE_TRACKERS)
//--------------------------------------------------------------
bool Utils::has_attribute(py::object& obj, const std::string &attr_name) {
	// Check if the object has the specified attribute
	if (PyObject_HasAttrString(obj.ptr(), attr_name.c_str())) {
		return true;
	}
	return false;
}

//--------------------------------------------------------------
bool Utils::has_all_attributes(py::object& obj, const std::vector<std::string>& attr_names) {
	for( auto& str : attr_names ) {
		// Check if the object has the specified attribute
		if (!PyObject_HasAttrString(obj.ptr(), str.c_str())) {
			return false;
		}
	}
	
	return true;
}

//--------------------------------------------------------------
void Utils::print_attributes(py::object& aobject) {
	if( !has_attribute(aobject, "__dict__")) {
//		std::cout << "-- ATTRIBUTES: --------------------------------- " << std::endl;
//		// Get a list of all attributes and methods of the object
//		py::list dir_result = py::module::import("__builtin__").attr("dir")(aobject);
//		
//		// Iterate over the attributes and methods and print them
//		for (auto item : dir_result) {
//			std::string name = py::str(item);
//			py::object attr = aobject.attr(name.c_str());
//			try {
//				// Try to call the attribute as a method
//				py::object result = attr();
//				std::cout << "Attribute/Method: " << name << std::endl;
//			} catch (...) {
//				// If it's not callable, it's likely an attribute
//				std::cout << "Attribute: " << name << std::endl;
//			}
//		}
		return;
	}

	// Let’s say you have a py::object, for example, a Python list 
	// py::object obj = py::eval("[]");
	// // Create an empty Python list// Get the dir() of the object, which lists its attributes and methods 
	// py::object dir_result = py::eval("dir")(obj);
	// // Convert the result to a Python list and print it 
	// for (auto item : dir_result) { std::cout << std::string(py::str(item)) << std::endl; }

	// Use Python’s inspect module to get all methods 
	py::module inspect = py::module::import("inspect");
	py::list methods = inspect.attr("getmembers")(aobject, inspect.attr("ismethod"));
	std::cout << "Methods found using inspect:\n";
	for (auto method : methods) {
		py::tuple tup = method.cast<py::tuple>();
		std::string name = tup[0].cast<std::string>();
		std::cout << " - " << name << std::endl;
	}

	std::cout << "-- ATTRIBUTES dictionary: --------------------------------- " << std::endl;
//	 Get the dictionary of the object and print it out so we can see what attributes are available.
	py::dict attributes = aobject.attr("__dict__");
//	 Loop through the dictionary
	for (auto item : attributes) {
		py::print(item.first, ":", item.second);
	}
}
#endif
