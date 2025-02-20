//
//  MediaPipeTracker.cpp
//  FaceDraw
//
//  Created by Theo on 2/8/24.
//

#include "ofxMediaPipeFaceTracker.h"
#if !defined(OFX_MEDIAPIPE_EXCLUDE_TRACKERS)
#include "ofxMediaPipeUtils.h"
#include "ofGraphics.h"

using namespace ofx::MediaPipe;

using std::string;
using std::shared_ptr;
using std::vector;
using std::make_shared;

//-------------------------------------------------
FaceTracker::FaceTracker() {
	mMinScoreForActiveBlendshape.set("MinScoreForActiveBlendshape", 0.4, 0.0f, 1.0f );
	mPosSmoothing.set("PosSmoothPct", 0.f, 0.f, 1.f);
	_initParams();
	mBDrawIrises.set("DrawIrises", true);
}

//-------------------------------------------------
FaceTracker::~FaceTracker() {
	Tracker::release();
}

//-------------------------------------------------
ofParameterGroup& FaceTracker::getParams() {
	if( params.size() < 1 ) {
		params.setName(mGuiPrefix+"-MediaPipeFaceTracker");
		
		_addToParams();
		params.add( mBDrawIrises );
//		params.add( mMaxDistToMatch );
		params.add(mMinScoreForActiveBlendshape);
		params.add(mPosSmoothing);
	}
	return params;
}

////-------------------------------------------------
//bool FaceTracker::setup(int maxNumFaces, float minDetectConf, float minTrackConf) {
//	Settings tsettings;
//	tsettings.maxNumFaces = maxNumFaces;
//	tsettings.minFaceDetectionConfidence = minDetectConf;
//	tsettings.minTrackingConfidence = minTrackConf;
//	return setup( tsettings );
//}

//-------------------------------------------------
bool FaceTracker::setup( const Tracker::Settings& asettings ) {
	return setup( FaceSettings(asettings) );
}

//-------------------------------------------------
bool FaceTracker::setup( const FaceSettings& asettings ) {
	if (!Py_IsInitialized()) {
		ofLogNotice("FaceTracker") << "initing PYTHON";
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
		mSettings.filePath = ofToDataPath("face_landmarker_v2_with_blendshapes.task", true);
		if( !ofFile::doesFileExist(mSettings.filePath) ) {
			// trying to load from addon folder
			mSettings.filePath = ofToDataPath("../../../../../addons/ofxMediaPipePython/tasks/face_landmarker_v2_with_blendshapes.task", true);
		}
	}
	
	if( !ofFile::doesFileExist(mSettings.filePath) ) {
		ofLogError("HandTracker :: setup") << "hand land marker task not found at " << mSettings.filePath;
		return false;
	}
	
	Tracker::release();
	mBExiting = false;
	
	py::gil_scoped_acquire acquire;
	py_mediapipe = py::module::import("mediapipe");
	
	py::object BaseOptions = py_mediapipe.attr("tasks").attr("BaseOptions");
	py::object FaceLandmarker = py_mediapipe.attr("tasks").attr("vision").attr("FaceLandmarker");
	py::object FaceLandmarkerOptions = py_mediapipe.attr("tasks").attr("vision").attr("FaceLandmarkerOptions");
	py::object VisionRunningMode = py_mediapipe.attr("tasks").attr("vision").attr("RunningMode");
	
//	std::string taskPath = ofToDataPath("face_landmarker_v2_with_blendshapes.task", true);
	// Create BaseOptions object
	// py::object base_options = BaseOptions.attr("__call__")(py::arg("model_asset_path") = mSettings.filePath.string(),
								// py::arg("delegate") = BaseOptions.attr("Delegate").attr("GPU"));
	py::object base_options = BaseOptions.attr("__call__")(py::arg("model_asset_path") = mSettings.filePath.string());
	// base_options = python.BaseOptions(model_asset_path = 'gesture_recognizer.task', delegate = python.BaseOptions.Delegate.GPU)

	
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
			ofLogNotice("Media pipe face tracker") << " error with the callback function.";
		}
	};
	
	std::string rmodeStr = Tracker::sGetStringForRunningMode(mSettings.runningMode);
	
	py::object options;
	if( mSettings.runningMode == Tracker::MODE_LIVE_STREAM ) {
		options = FaceLandmarkerOptions.attr("__call__")(
														 py::arg("base_options") = base_options,
														 py::arg("running_mode") = VisionRunningMode.attr(rmodeStr.c_str()),
														 py::arg("num_faces") = mSettings.maxNum,
														 py::arg("min_face_detection_confidence") = mSettings.minDetectionConfidence,
														 py::arg("min_face_presence_confidence") = mSettings.minPresenceConfidence,
														 py::arg("min_tracking_confidence") = mSettings.minTrackingConfidence,
														 py::arg("output_face_blendshapes") = mSettings.outputFaceBlendshapes ? 1 : 0,
//														 py::arg("output_facial_transformation_matrixes") = mSettings.outputFacialTransformationMatrices ? 1 : 0,
//														 py::arg("refinedLandmarks") = mSettings.refinedLandmarks ? 1 : 0,
														py::arg("result_callback") = py::cpp_function(process_results_lambda)
														 );
	} else {
		options = FaceLandmarkerOptions.attr("__call__")(
														 py::arg("base_options") = base_options,
														 py::arg("running_mode") = VisionRunningMode.attr(rmodeStr.c_str()),
														 py::arg("num_faces") = mSettings.maxNum,
														 py::arg("min_face_detection_confidence") = mSettings.minDetectionConfidence,
														 py::arg("min_face_presence_confidence") = mSettings.minPresenceConfidence,
														 py::arg("min_tracking_confidence") = mSettings.minTrackingConfidence,
														 py::arg("output_face_blendshapes") = mSettings.outputFaceBlendshapes ? 1 : 0
//														 py::arg("output_facial_transformation_matrixes") = mSettings.outputFacialTransformationMatrices ? 1 : 0
//														 py::arg("refinedLandmarks") = mSettings.refinedLandmarks ? 1 : 0
														 );
	}
	
	py_landmarker = FaceLandmarker.attr("create_from_options")(options);
	
	_addAppListeners();
	
//	if( getRunningMode() == Tracker::MODE_OF_VIDEO_THREAD ) {
		py::gil_scoped_release release; // add this to release the GIL
//	}
	
	Tracker::sNumPyInstances++;
	
	mBSetup = true;
	return true;
}

//----------------------------------------------------------------------
void FaceTracker::_update() {
	if( mSettings.runningMode == Tracker::MODE_LIVE_STREAM || mSettings.runningMode == Tracker::MODE_OF_VIDEO_THREAD) {
		if(mHasNewThreadValues.load() ) {
			std::lock_guard<std::mutex> lck(mMutex);
//			mFaces = mThreadedFaces;
			_matchFaces(mThreadedFaces, mFaces);
//			mThreadedFaces.clear();
			mHasNewThreadValues = false;
			mBHasNewData = true;
			mFpsCounter.newFrame();
		}
	}
	
	_calculateDeltatime();
	float deltaTime = mDeltaTimeSmoothed;
	
//	float deltaTime = ofClamp( ofGetLastFrameTime(), 1.f/5.f, 1.f/5000.f);
	
	float minScoreActive = mMinScoreForActiveBlendshape;
	for( auto& face : mFaces ) {
		face->age += deltaTime;
		auto& bshapes = face->getBlendShapes();
		for( auto& bshape : bshapes ) {
			bshape.bChangedToActive = false;
			if( bshape.score > minScoreActive ) {
//				if( bshape.timeInActive > 0.5f ) {
//					bshape.changed = true;
//				}
				bshape.timeActive += deltaTime;
				if( bshape.timeActive > 0.1f) {
					if( !bshape.bFiredActiveEvent ) {
						bshape.bFiredActiveEvent = true;
						bshape.bChangedToActive = true;
					}
				}
				bshape.timeInActive = 0.f;
			} else {
				bshape.timeActive = 0.f;
				bshape.timeInActive += deltaTime;
				if( bshape.timeInActive > 0.35f ) {
					bshape.bFiredActiveEvent = false;
				}
			}
		}
	}
}

//-------------------------------------------------
void FaceTracker::draw() {
	for( auto& face : mFaces ) {
		if( mBDrawPoints ) {
			face->drawPoints(mDrawPointSize, mBDrawUsePosZ);
		}
		if( mBDrawOutlines ) {
			face->drawOutlines(mBDrawUsePosZ);
		}
		if( mBDrawIrises ) {
			face->drawIrises(5.f, mBDrawUsePosZ);
		}
	}
}

//----------------------------------------------------------------------
void FaceTracker::draw(float x, float y, float w, float h) const {
	for( auto& face : mFaces ) {
//		face->drawPoints(2, x, y, w, h);
		ofPushMatrix(); {
			ofTranslate(x, y);
			ofScale(w, h, 1.f);
			face->getKeypointsNormalizedLineMesh().draw();
		} ofPopMatrix();
	}
}

//-------------------------------------------------
std::vector< std::shared_ptr<Face> >& FaceTracker::getFaces() {
	return mFaces;
}

////-------------------------------------------
//void FaceTracker::_process_results(py::object& aresults, py::object& aMpImage, int aTimestamp ) {
//	_process_landmark_results(aresults, aTimestamp);
//}

//-- ATTRIBUTES: ---------------------------------
//face_landmarks : [[NormalizedLandmark(x=0.5952834486961365, y=0.6115854382514954, z=-0.019650151953101158, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.6030879020690918, y=0.5429501533508301, z=-0.031443700194358826, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.588722288608551, y=0.5701600313186646, z=-0.017400220036506653, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.5816837549209595, y=0.49890798330307007, z=-0.02784018963575363, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.6024673581123352, y=0.5263262987136841, z=-0.032886654138565063, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.5955921411514282, y=0.5090116262435913, z=-0.029850613325834274, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.5733664035797119, y=0.4656454622745514, z=-0.012580784037709236, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.5044557452201843, y=0.47244399785995483, z=-0.03150442987680435, visibility=0.0, presence=0.0),
	// ............
//	NormalizedLandmark(x=0.5801571607589722, y=0.4489082098007202, z=0.040344227105379105, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.582961916923523, y=0.44768333435058594, z=0.04041014611721039, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.5787030458450317, y=0.4383198022842407, z=0.04041316360235214, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.5765012502670288, y=0.4502194821834564, z=0.04040399193763733, visibility=0.0, presence=0.0), NormalizedLandmark(x=0.5809922218322754, y=0.4599885940551758, z=0.04040698707103729, visibility=0.0, presence=0.0)]]
//face_blendshapes : []
//facial_transformation_matrixes : []

//face_blendshapes : [[Category(index=0, score=6.176930037327111e-05, display_name='', category_name='_neutral'), Category(index=1, score=0.8781865239143372, display_name='', category_name='browDownLeft'), Category(index=2, score=0.6900099515914917, display_name='', category_name='browDownRight'), Category(index=3, score=7.805385394021869e-05, display_name='', category_name='browInnerUp'), Category(index=4, score=0.00020061589020770043, display_name='', category_name='browOuterUpLeft'), Category(index=5, score=0.0020954988431185484, display_name='', category_name='browOuterUpRight'), Category(index=6, score=8.985885506263003e-05, display_name='', category_name='cheekPuff'), Category(index=7, score=3.3365580520694493e-07, display_name='', category_name='cheekSquintLeft'), Category(index=8, score=1.1590733492994332e-06, display_name='', category_name='cheekSquintRight'), Category(index=9, score=0.4877426028251648, display_name='', category_name='eyeBlinkLeft'), Category(index=10, score=0.06220365688204765, display_name='', category_name='eyeBlinkRight'), Category(index=11, score=0.1771458238363266, display_name='', category_name='eyeLookDownLeft'), Category(index=12, score=0.1494378298521042, display_name='', category_name='eyeLookDownRight'), Category(index=13, score=0.13252319395542145, display_name='', category_name='eyeLookInLeft'), Category(index=14, score=0.031330715864896774, display_name='', category_name='eyeLookInRight'), Category(index=15, score=0.05334152653813362, display_name='', category_name='eyeLookOutLeft'), Category(index=16, score=0.17473335564136505, display_name='', category_name='eyeLookOutRight'), Category(index=17, score=0.08254580199718475, display_name='', category_name='eyeLookUpLeft'), Category(index=18, score=0.11734944581985474, display_name='', category_name='eyeLookUpRight'), Category(index=19, score=0.863969624042511, display_name='', category_name='eyeSquintLeft'), Category(index=20, score=0.40399062633514404, display_name='', category_name='eyeSquintRight'), Category(index=21, score=0.0011884826235473156, display_name='', category_name='eyeWideLeft'), Category(index=22, score=0.009471593424677849, display_name='', category_name='eyeWideRight'), Category(index=23, score=0.00017672085959929973, display_name='', category_name='jawForward'), Category(index=24, score=0.006098506972193718, display_name='', category_name='jawLeft'), Category(index=25, score=0.011814729310572147, display_name='', category_name='jawOpen'), Category(index=26, score=2.4880477212718688e-05, display_name='', category_name='jawRight'), Category(index=27, score=0.0036693245638161898, display_name='', category_name='mouthClose'), Category(index=28, score=0.4390335977077484, display_name='', category_name='mouthDimpleLeft'), Category(index=29, score=0.006463940721005201, display_name='', category_name='mouthDimpleRight'), Category(index=30, score=0.009011869318783283, display_name='', category_name='mouthFrownLeft'), Category(index=31, score=0.046264082193374634, display_name='', category_name='mouthFrownRight'), Category(index=32, score=0.000153228611452505, display_name='', category_name='mouthFunnel'), Category(index=33, score=0.7059570550918579, display_name='', category_name='mouthLeft'), Category(index=34, score=0.00024998601293191314, display_name='', category_name='mouthLowerDownLeft'), Category(index=35, score=0.0002560523571446538, display_name='', category_name='mouthLowerDownRight'), Category(index=36, score=0.1478787362575531, display_name='', category_name='mouthPressLeft'), Category(index=37, score=0.08702187240123749, display_name='', category_name='mouthPressRight'), Category(index=38, score=0.00025747762992978096, display_name='', category_name='mouthPucker'), Category(index=39, score=5.744654885120326e-08, display_name='', category_name='mouthRight'), Category(index=40, score=0.11254341155290604, display_name='', category_name='mouthRollLower'), Category(index=41, score=0.05102065950632095, display_name='', category_name='mouthRollUpper'), Category(index=42, score=0.3245639503002167, display_name='', category_name='mouthShrugLower'), Category(index=43, score=0.02399911917746067, display_name='', category_name='mouthShrugUpper'), Category(index=44, score=0.06230587512254715, display_name='', category_name='mouthSmileLeft'), Category(index=45, score=0.03185177966952324, display_name='', category_name='mouthSmileRight'), Category(index=46, score=0.0015488178469240665, display_name='', category_name='mouthStretchLeft'), Category(index=47, score=0.4855349361896515, display_name='', category_name='mouthStretchRight'), Category(index=48, score=0.001965946750715375, display_name='', category_name='mouthUpperUpLeft'), Category(index=49, score=0.0003429520584177226, display_name='', category_name='mouthUpperUpRight'), Category(index=50, score=2.885544176933763e-07, display_name='', category_name='noseSneerLeft'), Category(index=51, score=9.393374057253823e-06, display_name='', category_name='noseSneerRight')]]

//facial_transformation_matrixes : [array([[ 9.95370328e-01,  8.79315287e-02, -3.88101824e-02,
//										   1.01756287e+00],
//										 [-8.31988528e-02,  9.90424037e-01,  1.10173352e-01,
//										   2.10897541e+00],
//										 [ 4.81262505e-02, -1.06434323e-01,  9.93154526e-01,
//										   -3.98050499e+01],
//										 [ 0.00000000e+00,  0.00000000e+00,  0.00000000e+00,
//										   1.00000000e+00]])]

//----------------------------------------------------------
void FaceTracker::_process_landmark_results(py::object& aresults, int aTimestamp) {
	if (mBExiting.load()) {
		return;
	}
	
	if (aresults.is_none()) {
		ofLogNotice("FaceTracker::_process_landmark_results") << "Results are bad, returning. " << aTimestamp;
		return;
	}
	
//	Utils::print_attributes(aresults);
//	return;
	
	if( !Utils::has_attribute(aresults, "face_landmarks")) {
		ofLogError("FaceTracker::_process_landmark_results") << "Does not contain either face_landmarks or face_world_landmarks attributes. " << aTimestamp;
		return;
	}
	
	//https://developers.google.com/mediapipe/solutions/vision/hand_landmarker/python#live-stream_2
	std::vector< std::shared_ptr<Face> > tfaces;
	try {
//		py::gil_scoped_acquire acquire;
		py::list face_landmarks_list = aresults.attr("face_landmarks").cast<py::list>();
		py::list face_blendshapes_list;
		py::list face_trans_mats_list;

		//	Check if results is iterable
		if (!py::isinstance<py::iterable>(face_landmarks_list)) {
			ofLogError("FaceTracker::_process_landmark_results") << "'results' face_landmarks is not iterable." << aTimestamp;
			//py::gil_scoped_release release;
			return;
		}

		if (mSettings.outputFaceBlendshapes) {
			if (!Utils::has_attribute(aresults, "face_blendshapes")) {
				ofLogError("FaceTracker::_process_landmark_results") << "Does not contain face_blendshapes attributes." << aTimestamp;
				//py::gil_scoped_release release;
				return;
			}

			face_blendshapes_list = aresults.attr("face_blendshapes").cast<py::list>();

			//	Check if results are iterable
			if (!py::isinstance<py::iterable>(face_blendshapes_list)) {
				ofLogError("FaceTracker::_process_landmark_results") << "'results' face_blendshapes is not iterable." << aTimestamp;
				//py::gil_scoped_release release;
				return;
			}
		}

		//	if( mSettings.outputFacialTransformationMatrices ) {
		//		if( !Utils::has_attribute(aresults, "facial_transformation_matrixes")) {
		//			ofLogError("FaceTracker::_process_landmark_results") << "Does not contain facial_transformation_matrixes attributes." << aTimestamp;
		//			return;
		//		}
		//		
		//		face_trans_mats_list = aresults.attr("facial_transformation_matrixes").cast<py::list>();
		//		
		//		//	Check if results is iterable
		//		if (!py::isinstance<py::iterable>(face_trans_mats_list)) {
		//			ofLogError("FaceTracker::_process_landmark_results") << "'results' facial_transformation_matrixes is not iterable." << aTimestamp;
		//			return;
		//		}
		//	}

			// Acquire GIL before interacting with Python objects 
			//py::gil_scoped_acquire acquire;


		//	std::cout << "_________________________________________" << std::endl;
		// std::vector< std::shared_ptr<Face> > tfaces;
		int numMarks = py::len(face_landmarks_list);
		for (int i = 0; i < numMarks; i++) {

			//		Face tface;
			auto tface = std::make_shared<Face>();

			py::list face_landmarks = face_landmarks_list[i];
			//		py::list face_world_landmarks = face_world_landmarks_list[i];

			if (mSettings.outputFaceBlendshapes) {
				py::list face_blendshapes = face_blendshapes_list[i];
				int numBlends = py::len(face_blendshapes);
				for (int j = 0; j < numBlends; j++) {
					py::object hbs = face_blendshapes[j];
					std::string hcategory = py::str(hbs.attr("category_name"));
					float hscore = py::float_(hbs.attr("score"));
					int hindex = py::int_(hbs.attr("index"));
					//				std::cout << "index: " << hindex << " hcategory: " << hcategory << " score: " << hscore << std::endl;
					tface->setIncomingBlendShape(hcategory, hscore, hindex);
				}
			}

			//		if( mSettings.outputFacialTransformationMatrices ) {
			////			py::array face_matrices = py::array(face_trans_mats_list[i]);
			////			float f1 = py::float_(face_matrices[0][0]);
			////			cout << "face matrices: " << f1 << std::endl;
			//		}

			int num = py::len(face_landmarks);

			tface->keypoints.assign(num, TrackedObject::Keypoint());

			//		for( int j = 0; j < num; j++ ) {
			//			py::object hl = face_landmarks[j];
			int j = 0;
			for (py::handle hl : face_landmarks) {
				//			py::object hwl = face_world_landmarks[j];

				auto& kp = tface->keypoints[j];
				//			tface.keypoints[j].pos = glm::vec3(py::float_(hl.attr("x")), py::float_(hl.attr("y")), py::float_(hl.attr("z")) );
				kp.pos.x = py::float_(py::getattr(hl, "x"));
				kp.pos.y = py::float_(py::getattr(hl, "y"));
				kp.pos.z = py::float_(py::getattr(hl, "z"));

				kp.posN = kp.pos;
				kp.pos.x *= mOutRect.width;
				kp.pos.y *= mOutRect.height;
				kp.pos.x += mOutRect.x;
				kp.pos.y += mOutRect.y;

				kp.pos.z *= mOutRect.width;

				//			thand.keypoints[j].posWorld = glm::vec3(py::float_(hwl.attr("x")), py::float_(hwl.attr("y")), py::float_(hwl.attr("z")) );
							//			thand.keypoints[j].worldPos.z *= mOutRect.width;

				//			kp.visibility = py::float_(hl.attr("visibility"));
				//			kp.presence = py::float_(hl.attr("presence"));

				j++;
			}

			tfaces.push_back(tface);
		}
		//py::gil_scoped_release release;
	} catch (...) {
		ofLogError("FaceTracker") << __FUNCTION__ << " error with gil";
		return;
	}

	// Release GIL while performing intensive C++ operations
	//py::gil_scoped_release release;		

	
	if( mSettings.runningMode == Tracker::MODE_LIVE_STREAM) {
		std::lock_guard<std::mutex> lck(mMutex);
		//		_matchFaces( tfaces, mThreadedFaces );
		mThreadedFaces = tfaces;
		mHasNewThreadValues = true;
	} else if( mSettings.runningMode == Tracker::MODE_OF_VIDEO_THREAD ) {
		std::lock_guard<std::mutex> lck(mMutex);
		mThreadedFaces = tfaces;
		mHasNewThreadValues = true;
	} else {
		_matchFaces( tfaces, mFaces );
	}
}

//-----------------------------------------------------------------------
void FaceTracker::_matchFaces( std::vector<std::shared_ptr<Face>>& aIncomingFaces, std::vector< std::shared_ptr<Face>>& aFaces ) {
	
	float frameRate = ofClamp(Tracker::mFpsCounter.getFps(), 1, 200);
	int mNumFramesToDie = Tracker::mMaxTimeToMatch * frameRate;
	
	// now lets try to track the hands
	for( auto& face : aFaces ) {
		face->trackingData.bFoundThisFrame = false;
		face->trackingData.numFramesNotFound++;
		if( face->trackingData.numFramesNotFound > mNumFramesToDie ) {
			face->bRemove = true;
		}
	}
	
	ofRemove( aFaces, TrackedObject::shouldRemove );
	
	float maxDistToMatch2 = mMaxDistToMatch * mMaxDistToMatch;
	//	glm::vec3 posN = {0.f, 0.f, 0.f };
	// now lets try to find a hand to match //
	for( auto& aInFace : aIncomingFaces ) {
		// try to match with current hands
//		bool bFoundAMatch = false;
		std::shared_ptr<Face> bestMatch;
		float closestHandDistMatch = 9999. * 8874.f;
		for( auto& face : aFaces ) {
			face->trackingData.matchDistance = 0.f;
			if( face->trackingData.bFoundThisFrame ) {continue;}
			// simple distance calculation //
			// to determine if within realm of match
//			float dist2 = glm::distance2( face->getPositionNormalized(), aInFace->getPositionNormalized() );
			float dist = glm::distance( face->getPositionNormalized(), aInFace->getPositionNormalized() );
//			if( dist2 < maxDistToMatch2 ) {
			if( dist < mMaxDistToMatch ) {
				// possible hand match //
//				face->trackingData.matchDistance += dist2;
				face->trackingData.matchDistance += dist;
				
				// figure out some more confidence match here
				if( dist < closestHandDistMatch ) {
					bestMatch = face;
					closestHandDistMatch = dist;
//					closestHandDistMatch = face->trackingData.matchDistance;
				}
			}
		}
		
		shared_ptr<Face> myFace;
		if( bestMatch ) {
			bestMatch->trackingData.bFoundThisFrame = true;
			bestMatch->trackingData.numFramesNotFound = 0;
//			bestMatch->keypoints = tface->keypoints;
			myFace = bestMatch;
		} else {
			//ofLogNotice("MediaPipeFaceTracker:: ") << "creating a new face | " << ofGetFrameNum();
			// we need to add a face //
			myFace = std::make_shared<Face>( *(aInFace.get()));
			myFace->ID = (mCounterId++);
			aFaces.push_back(myFace);
		}
		
		if( myFace ) {
			if( mPosSmoothing < 0.001f || mPosSmoothing > 0.99f ){
				myFace->updateFrom(aInFace);
			} else {
				myFace->updateFromFaceWithSmoothing(aInFace, mPosSmoothing);
			}
//			myFace->updateFromKeypoints();
//			myFace->updateBlendShapes(tface->getIncomingBlendShapes());
		}
	}
}
#endif
