//
//  MediaPipeTracker.cpp
//  FaceDraw
//
//  Created by Theo on 2/8/24.
//

#include "ofxMediaPipeHandTracker.h"
#if !defined(OFX_MEDIAPIPE_EXCLUDE_TRACKERS)
#include "ofGraphics.h"
#include "of3dUtils.h"
#include "ofxMediaPipeUtils.h"

//Landmarks
//
//There are 21 hand landmarks, each composed of x, y and z coordinates. The x and y coordinates are normalized to [0.0, 1.0] by the image width and height, respectively. The z coordinate represents the landmark depth, with the depth at the wrist being the origin. The smaller the value, the closer the landmark is to the camera. The magnitude of z uses roughly the same scale as x.

//World Landmarks
//
//The 21 hand landmarks are also presented in world coordinates. Each landmark is composed of x, y, and z, representing real-world 3D coordinates in meters with the origin at the hand’s geometric center.

using std::string;
using std::shared_ptr;
using std::vector;
using std::make_shared;

using namespace ofx::MediaPipe;

//ofParameter<float> HandTracker::mMaxDegForOpenFinger;


////-------------------------------------------
//void HandTracker::process_results(py::object& aresults, py::object& aMpImage, int aTimestamp ) {
//	_process_landmark_results(aresults, aTimestamp);
//}




//----------------------------------------------------------
HandTracker::HandTracker() {
//	mMaxDistToMatch.set("MaxDistToMatchHands", 0.1f, 0.0, 1.f);
	mMaxDegForOpenFinger.set("MaxDegForOpenFinger", 60.0f, 0.0f, 90.f);
	mPosSmoothing.set("PosSmoothPct", 0.f, 0.f, 1.f);
	
	_initParams();
	
	mBDrawFingers.set("DrawFingers", true );
	mBDrawFingerInfo.set("DrawFingerInfo", false );
}

//----------------------------------------------------------
HandTracker::~HandTracker() {
	Tracker::release();
}

//----------------------------------------------------------
ofParameterGroup& HandTracker::getParams() {
	if( params.size() < 1 ) {
		params.setName(mGuiPrefix+"MediaPipeHandTracker");
		
		_addToParams();
		params.add(mBDrawFingers);
		params.add(mBDrawFingerInfo);
		
//		params.add( mMaxDistToMatch );
		params.add( mMaxDegForOpenFinger );
		params.add(mPosSmoothing);
	}
	return params;
}

//----------------------------------------------------------
bool HandTracker::setup(const Tracker::Settings& asettings) {
	return setup( HandSettings(asettings));
}

//----------------------------------------------------------
bool HandTracker::setup(const HandSettings& asettings) {
	if (!Py_IsInitialized()) {
		ofLogNotice("HandTracker") << "initing PYTHON";
		py::initialize_interpreter();
	}
	
	if( mBSetup ) {
		Tracker::release();
	}
	
	mSettings = asettings;
	mHasNewThreadValues = false;
	mBSetup = false;
	
	//	std::string taskPath = ofToDataPath("hand_landmarker.task", true);
	if( mSettings.filePath.empty() ) {
		mSettings.filePath = ofToDataPath("hand_landmarker.task", true);
		if( !ofFile::doesFileExist(mSettings.filePath) ) {
			// trying to load from addon folder
			mSettings.filePath = ofToDataPath("../../../../../addons/ofxMediaPipePython/tasks/hand_landmarker.task", true);
		}
	}
	
	if( !ofFile::doesFileExist(mSettings.filePath) ) {
		ofLogError("HandTracker :: setup") << "hand land marker task not found at " << mSettings.filePath;
		return false;
	}
	// release py objects, just in case
	Tracker::release();
	
	mBExiting = false;
	
	py::gil_scoped_acquire acquire;
	
	py_mediapipe = py::module::import("mediapipe");
	
	py::object BaseOptions = py_mediapipe.attr("tasks").attr("BaseOptions");
	py::object HandLandmarker = py_mediapipe.attr("tasks").attr("vision").attr("HandLandmarker");
	py::object HandLandmarkerOptions = py_mediapipe.attr("tasks").attr("vision").attr("HandLandmarkerOptions");
//	py::object HandLandmarkerResult = py_mediapipe.attr("tasks").attr("vision").attr("HandLandmarkerResult");
	py::object VisionRunningMode = py_mediapipe.attr("tasks").attr("vision").attr("RunningMode");
	
	// Create BaseOptions object
	py::object base_options = BaseOptions.attr("__call__")(py::arg("model_asset_path") = mSettings.filePath.string());
	
	process_results_lambda = [this](py::object& aresults, py::object& aMpImage, int aTimestamp) {
		//		std::cout << "Class Lambda Result callback called. " << aTimestamp << std::endl;
//		_process_results_callback( aresults, aMpImage, aTimestamp );
		if( mBExiting.load() ) {
			mThreadCallCount--;
			if( mThreadCallCount < 0 ) {
				mThreadCallCount = 0;
			}
			return;
		}
		try {
			_process_results_callback( aresults, aMpImage, aTimestamp );
		} catch(...) {
			//			ofLogNotice(getTrackerTypeAsString()) << " error with the callback function.";
//			std::lock_guard<std::mutex> lck(mMutexMediaPipe);
//			mThreadCallCount--;
//			if( mThreadCallCount < 0 ) {
//				mThreadCallCount = 0;
//			}
			ofLogNotice("Media pipe hand tracker") << " error with the callback function.";
		}
	};
	
	std::string rmodeStr = Tracker::sGetStringForRunningMode(mSettings.runningMode);
	
	py::object options;
	if( mSettings.runningMode == Tracker::MODE_LIVE_STREAM ) {
		options = HandLandmarkerOptions.attr("__call__")(
														 py::arg("base_options") = base_options,
														 py::arg("running_mode") = VisionRunningMode.attr(rmodeStr.c_str()),
														 py::arg("num_hands") = mSettings.maxNum,
														 py::arg("min_hand_detection_confidence") = mSettings.minDetectionConfidence,
														 py::arg("min_tracking_confidence") = mSettings.minTrackingConfidence,
														 py::arg("min_hand_presence_confidence") = mSettings.minPresenceConfidence,
														 py::arg("result_callback") = py::cpp_function(process_results_lambda)
														 );
	} else {
		options = HandLandmarkerOptions.attr("__call__")(
														 py::arg("base_options") = base_options,
														 py::arg("running_mode") = VisionRunningMode.attr(rmodeStr.c_str()),
														 py::arg("num_hands") = mSettings.maxNum,
														 py::arg("min_hand_detection_confidence") = mSettings.minDetectionConfidence,
														 py::arg("min_tracking_confidence") = mSettings.minTrackingConfidence,
														 py::arg("min_hand_presence_confidence") = mSettings.minPresenceConfidence
														 );
	}
		
		
	py_landmarker = HandLandmarker.attr("create_from_options")(options);
	
	_addAppListeners();
	
//	if( getRunningMode() == Tracker::MODE_OF_VIDEO_THREAD ) {
		py::gil_scoped_release release; // add this to release the GIL
//	}
	
	Tracker::sNumPyInstances++;
	
	mBSetup = true;
	return true;
}

//----------------------------------------------------------------------
void HandTracker::_update() {
	if( mSettings.runningMode == Tracker::MODE_LIVE_STREAM || mSettings.runningMode == Tracker::MODE_OF_VIDEO_THREAD) {
		if(mHasNewThreadValues.load() ) {
			std::lock_guard<std::mutex> lck(mMutex);
//			mHands = mThreadedHands;
			_matchHands(mThreadedHands, mHands);
//			mThreadedHands.clear();
			mHasNewThreadValues = false;
			mBHasNewData = true;
			mFpsCounter.newFrame();
		}
	}
	
	_calculateDeltatime();
	float deltaTime = mDeltaTimeSmoothed;
	
	for( auto& hand : mHands ) {
		hand->age += deltaTime;
		hand->setMaxDegreesForOpenFinger( mMaxDegForOpenFinger );
		for( auto& finger : hand->fingers ) {
			if( finger->isOpen(mMaxDegForOpenFinger)) {
				finger->timeOpen += deltaTime;
				finger->timeClosed = 0.f;
			} else {
				finger->timeOpen = 0.0f;
				finger->timeClosed += deltaTime;
			}
		}
	}
}

//----------------------------------------------------------------------
void HandTracker::draw() {
	for( auto& hand : mHands ) {
		if( mBDrawPoints ) {
			hand->drawPoints(mDrawPointSize, mBDrawUsePosZ);
		}
		if( mBDrawOutlines ) {
//			ofSetColor(255);
			hand->drawOutlines(mBDrawUsePosZ);
		}
		if( mBDrawFingers ) {
			hand->drawFingers(mBDrawUsePosZ);
		}
		if( mBDrawFingerInfo ) {
			hand->drawFingersInfo(mBDrawUsePosZ);
		}
	}
}

//----------------------------------------------------------------------
void HandTracker::draw(float x, float y, float w, float h) const {
	for( auto& hand : mHands ) {
//		hand->drawPoints(2, x, y, w, h);
		ofPushMatrix(); {
			ofTranslate(x, y);
			ofScale(w, h, 1.f);
			hand->getKeypointsNormalizedLineMesh().draw();
		} ofPopMatrix();
		
	}
}

//----------------------------------------------------------------------
std::vector< std::shared_ptr<Hand> >& HandTracker::getHands() {
	return mHands;
}

//handedness : [[Category(index=1, score=0.7853734493255615, display_name='Left', category_name='Left')]]
//hand_landmarks : [[NormalizedLandmark(x=0.7700628638267517, y=0.802154541015625, z=2.99581245144509e-07, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.7269693613052368, y=0.7757196426391602, z=-0.018525315448641777, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.6862121224403381, y=0.7282775044441223, z=-0.030217792838811874, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.6526286602020264, y=0.6956480145454407, z=-0.0407099649310112, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.6229178309440613, y=0.6846016049385071, z=-0.0523398220539093, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.7248960733413696, y=0.6219297647476196, z=-0.026713712140917778, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.7118009328842163, y=0.5393090844154358, z=-0.041377097368240356, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.7054756283760071, y=0.4881035387516022, z=-0.052136726677417755, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.7016363143920898, y=0.4442553222179413, z=-0.06066303327679634, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.7582827210426331, y=0.6115716695785522, z=-0.030016737058758736, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.7587210536003113, y=0.5174922347068787, z=-0.042694445699453354, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.7611851096153259, y=0.4583998918533325, z=-0.05297582969069481, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.7650063037872314, y=0.4089915454387665, z=-0.0617927648127079, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.7887251973152161, y=0.6221291422843933, z=-0.035371795296669006, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.7997431755065918, y=0.5375277400016785, z=-0.053111813962459564, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.8079150319099426, y=0.48467159271240234, z=-0.06526942551136017, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.8148285150527954, y=0.43825966119766235, z=-0.0740494653582573, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.81569904088974, y=0.6497688293457031, z=-0.041857898235321045, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.8403620719909668, y=0.59648197889328, z=-0.060594040900468826, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.8580101132392883, y=0.5622614622116089, z=-0.06964937597513199, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.8728396892547607, y=0.5299782156944275, z=-0.07583682239055634, visibility=0.0, presence=0.0)]]
//hand_world_landmarks : [[Landmark(x=0.006685516331344843, y=0.07610618323087692, z=0.040495019406080246, visibility=0.0, presence=0.0), Landmark(x=-0.022024918347597122, y=0.05909471958875656, z=0.018718942999839783, visibility=0.0, presence=0.0), Landmark(x=-0.045786432921886444, y=0.04494400694966316, z=0.01305803656578064, visibility=0.0, presence=0.0), Landmark(x=-0.07290252298116684, y=0.02922983467578888, z=0.00031625479459762573, visibility=0.0, presence=0.0), Landmark(x=-0.09517879039049149, y=0.016235588118433952, z=-0.003068894147872925, visibility=0.0, presence=0.0), Landmark(x=-0.029854686930775642, y=0.0006955352146178484, z=0.0006534606218338013, visibility=0.0, presence=0.0), Landmark(x=-0.03406838700175285, y=-0.026650594547390938, z=-0.008894145488739014, visibility=0.0, presence=0.0), Landmark(x=-0.03962825983762741, y=-0.04460236802697182, z=-0.01850660890340805, visibility=0.0, presence=0.0), Landmark(x=-0.04587438330054283, y=-0.05410303920507431, z=-0.047971874475479126, visibility=0.0, presence=0.0), Landmark(x=-0.00511194858700037, y=-0.003661972237750888, z=0.003855682909488678, visibility=0.0, presence=0.0), Landmark(x=-0.005253266543149948, y=-0.037635669112205505, z=-0.01105014979839325, visibility=0.0, presence=0.0), Landmark(x=-0.009853962808847427, y=-0.05410832539200783, z=-0.033931247889995575, visibility=0.0, presence=0.0), Landmark(x=-0.011906065978109837, y=-0.07113976776599884, z=-0.04982277750968933, visibility=0.0, presence=0.0), Landmark(x=0.019932344555854797, y=-0.002301717409864068, z=0.0006671473383903503, visibility=0.0, presence=0.0), Landmark(x=0.022409604862332344, y=-0.029750555753707886, z=-0.011924408376216888, visibility=0.0, presence=0.0), Landmark(x=0.02385624498128891, y=-0.04673103243112564, z=-0.031734734773635864, visibility=0.0, presence=0.0), Landmark(x=0.020600594580173492, y=-0.061787888407707214, z=-0.050707537680864334, visibility=0.0, presence=0.0), Landmark(x=0.03504936769604683, y=0.012213998474180698, z=-0.0012091100215911865, visibility=0.0, presence=0.0), Landmark(x=0.04779692366719246, y=-0.0056094881147146225, z=-0.004539430141448975, visibility=0.0, presence=0.0), Landmark(x=0.05846109613776207, y=-0.020058535039424896, z=-0.0188915953040123, visibility=0.0, presence=0.0), Landmark(x=0.060523319989442825, y=-0.029996195808053017, z=-0.033676162362098694, visibility=0.0, presence=0.0)]]

//----------------------------------------------------------
void HandTracker::_process_landmark_results(py::object& aresults, int aTimestamp) {
	if (mBExiting.load()) {
		return;
	}
	
	if (aresults.is_none()) {
		ofLogNotice("HandTracker::_process_landmark_results") << "Results are bad, returning. " << aTimestamp;
		return;
	}
	
	//	cout << "handmark results has attribute landmarks: " << has_attribute( aresults, "landmark") << endl;
	//	cout << "handmark results has attribute hand_landmarks: " << has_attribute( aresults, "hand_landmarks") << endl;
	//	cout << "handmark results has attribute hand_world_landmarks: " << has_attribute( aresults, "hand_world_landmarks") << endl;
	
	//	cout << "ATTRIBUTES: --------------------------------- " << endl;
	////	 Get the dictionary of the object and print it out so we can see what attributes are available.
	//	py::dict attributes = aresults.attr("__dict__");
	////	 Loop through the dictionary
	//	for (auto item : attributes) {
	//		py::print(item.first, ":", item.second);
	//	}
	std::vector< std::shared_ptr<Hand> > thands;
	try {
//		py::gil_scoped_acquire acquire;

		//	if( !Utils::has_attribute(aresults, "hand_landmarks") || !Utils::has_attribute(aresults, "hand_world_landmarks") || !Utils::has_attribute(aresults, "handedness")) {
		if (!Utils::has_all_attributes(aresults, { "hand_landmarks", "hand_world_landmarks", "handedness" })) {
			ofLogError("HandTracker::_process_landmark_results") << "Does not contain either hand_landmarks or hand_world_landmarks or handedness attributes. " << aTimestamp;
			//py::gil_scoped_release release;
			return;
		}

		py::list hand_landmarks_list = aresults.attr("hand_landmarks").cast<py::list>();
		py::list hand_world_landmarks_list = aresults.attr("hand_world_landmarks").cast<py::list>();;
		py::list handedness_list = aresults.attr("handedness").cast<py::list>();;

		//	Check if results is iterable
		if (!py::isinstance<py::iterable>(hand_landmarks_list)) {
			ofLogError("HandTracker::_process_landmark_results") << "'results' hand_landmarks is not iterable. " << aTimestamp;
			//py::gil_scoped_release release;
			return;
		}

		//https://developers.google.com/mediapipe/solutions/vision/hand_landmarker/python#live-stream_2

		// Acquire GIL before interacting with Python objects 
		//py::gil_scoped_acquire acquire;

		// std::vector< std::shared_ptr<Hand> > thands;
		int numMarks = py::len(hand_landmarks_list);
		for (int i = 0; i < numMarks; i++) {

			//		Hand thand;
			auto thand = std::make_shared<Hand>();

			py::list handedness = handedness_list[i];
			if (py::len(handedness) > 0) {
				py::object handed = handedness[0];
				// (index=1, score=0.9968175888061523, display_name='Left', category_name='Left')
				std::string hcategory = py::str(handed.attr("category_name"));
				//			cout << "Handed: " << hcategory << endl;

				if (ofToLower(hcategory) == "left") {
					thand->handed = Hand::Handedness::LEFT;
				}
				thand->index = py::int_(handed.attr("index"));
			}


			py::list hand_landmarks = hand_landmarks_list[i];
			py::list hand_world_landmarks = hand_world_landmarks_list[i];

			int num = py::len(hand_landmarks);

			thand->keypoints.assign(num, TrackedObject::Keypoint());

			//		for( int j = 0; j < num; j++ ) {
			int j = 0;
			for (py::handle hl : hand_landmarks) {
				//			py::object hl = hand_landmarks[j];
				//			py::object hwl = hand_world_landmarks[j];

				auto& kp = thand->keypoints[j];
				//			thand.keypoints[j].pos = glm::vec3(py::float_(hl.attr("x")), py::float_(hl.attr("y")), py::float_(hl.attr("z")) );
				kp.pos.x = py::float_(py::getattr(hl, "x"));
				kp.pos.y = py::float_(py::getattr(hl, "y"));
				kp.pos.z = py::float_(py::getattr(hl, "z"));

				kp.posN = kp.pos;
				kp.pos.x *= mOutRect.width;
				kp.pos.y *= mOutRect.height;
				kp.pos.x += mOutRect.x;
				kp.pos.y += mOutRect.y;

				kp.pos.z *= mOutRect.width;

				//			kp.visibility = py::float_(hl.attr("visibility"));
				//			kp.presence = py::float_(hl.attr("presence"));
				j++;
			}

			j = 0;
			for (py::handle hwl : hand_world_landmarks) {
				//			thand.keypoints[j].posWorld = glm::vec3(py::float_(hwl.attr("x")), py::float_(hwl.attr("y")), py::float_(hwl.attr("z")) );
				auto& kp = thand->keypoints[j];
				kp.posWorld.x = py::float_(py::getattr(hwl, "x"));
				kp.posWorld.y = py::float_(py::getattr(hwl, "y"));
				kp.posWorld.z = py::float_(py::getattr(hwl, "z"));
				j++;
			}

			//		if(thand.keypoints.size() > 1 ) {
			//			thand.pos = thand.keypoints[0].pos;
			//			thand.posN = thand.keypoints[0].posN;
			//			thand.posWorld = thand.keypoints[0].posWorld;
			//		}
					//
					////		https://developers.google.com/mediapipe/solutions/vision/hand_landmarker/python#video_2
					////		The 21 hand landmarks are also presented in world coordinates. Each landmark is composed of x, y, and z, representing real-world 3D coordinates in meters with the origin at the hand’s geometric center.
					//		for( int j = 1; j < thand.keypoints.size(); j++ ) {
					//			thand.keypoints[j].posWorld += thand.posWorld;
					//		}

			thands.push_back(thand);

			//		std::string attr_name = py::str(handedness_list[i]);
			//		std::cout << "Attribute/Method " << i << " / " << numMarks << " : num land marks: " << py::len(hand_landmarks) << std::endl;
			//		std::cout << " output: " << attr_name << endl;
		}

		//py::gil_scoped_release release;

	} catch (...) {
		ofLogError("HandTracker") << __FUNCTION__ << " error with gil";
		return;
	}

	// Release GIL while performing intensive C++ operations
	//py::gil_scoped_release release;
		
	
	if( mSettings.runningMode == Tracker::MODE_LIVE_STREAM) {
		std::lock_guard<std::mutex> lck(mMutex);
		mThreadedHands = thands;
		mHasNewThreadValues = true;
	} else if( mSettings.runningMode == Tracker::MODE_OF_VIDEO_THREAD ) {
		std::lock_guard<std::mutex> lck(mMutex);
		mThreadedHands = thands;
		mHasNewThreadValues = true;
	} else {
		_matchHands( thands, mHands );
	}
}

//-----------------------------------------------------------------------
void HandTracker::_matchHands( std::vector<std::shared_ptr<Hand>>& aIncomingHands, std::vector<std::shared_ptr<Hand>> & aHands ) {
	float frameRate = ofClamp(Tracker::mFpsCounter.getFps(), 1, 200);
	int mNumFramesToDie = Tracker::mMaxTimeToMatch * frameRate;
	
	// now lets try to track the hands
	for( auto& hand : aHands ) {
		hand->trackingData.bFoundThisFrame = false;
		hand->trackingData.numFramesNotFound++;
		if( hand->trackingData.numFramesNotFound > mNumFramesToDie ) {
			hand->bRemove = true;
		}
	}
	
	ofRemove( aHands, TrackedObject::shouldRemove );
	
	float maxDistToMatch2 = mMaxDistToMatch * mMaxDistToMatch;
	//	glm::vec3 posN = {0.f, 0.f, 0.f };
	// now lets try to find a hand to match //
	for( auto& thand : aIncomingHands ) {
		// try to match with current hands
//		bool bFoundAMatch = false;
		std::shared_ptr<Hand> bestMatch;
		float closestHandDistMatch = 9999. * 8874.f;
		for( auto& hand : aHands ) {
			hand->trackingData.matchDistance = 0.f;
			if( hand->trackingData.bFoundThisFrame ) {continue;}
			// simple distance calculation //
			// to determine if within realm of match
//			float dist2 = glm::distance2( hand->getPositionNormalized(), thand->getPositionNormalized() );
			float dist = glm::distance( hand->getPositionNormalized(), thand->getPositionNormalized() );
			if( dist < mMaxDistToMatch ) {
				// possible hand match //
				hand->trackingData.matchDistance += dist;
				
				// figure out some more confidence match here
				if( dist < closestHandDistMatch ) {
					bestMatch = hand;
					closestHandDistMatch = dist;//hand->trackingData.matchDistance;
				}
			}
		}
		
		shared_ptr<Hand> myHand;
		if( bestMatch ) {
			bestMatch->trackingData.bFoundThisFrame = true;
			bestMatch->trackingData.numFramesNotFound = 0;
//			bestMatch->keypoints = thand->keypoints;
//			bestMatch->handed = thand->handed;
//			bestMatch->index = thand->index;
			myHand = bestMatch;
		} else {
			// we need to add a hand //
			myHand = std::make_shared<Hand>( *(thand.get()));
			myHand->ID = (mCounterId++);
			aHands.push_back(myHand);
		}
		
		if( myHand ) {
			if( mPosSmoothing < 0.001f || mPosSmoothing > 0.99f ){
				myHand->updateFrom(thand);
			} else {
				myHand->updateFromHandWithSmoothing(thand, mPosSmoothing);
			}
//			myHand->updateFromKeypoints();
//			myHand->updateFingers();
		}
	}
}
#endif
