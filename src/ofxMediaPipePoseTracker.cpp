//
//  ofxMediaPipePoseTracker.cpp
//  PoseTrackerDemo
//
//  Created by Nick Hardeman on 4/30/24.
//

#include "ofxMediaPipePoseTracker.h"
#if !defined(OFX_MEDIAPIPE_EXCLUDE_TRACKERS)
#include "ofxMediaPipeUtils.h"
#include "ofGraphics.h"

using namespace ofx::MediaPipe;

using std::string;
using std::shared_ptr;
using std::vector;
using std::make_shared;

//--------------------------------------------------------------
PoseTracker::PoseTracker() {
	_initParams();
	mBDrawMask.set("DrawMask", false);
	mPosSmoothing.set("PosSmoothPct", 0.f, 0.f, 1.f);
//	mBFilterMatchingFaces.set("FilterMatchingFaces", false);
//	mMatchFaceDistance.set("MatchFaceDistance", 0.05f, 0.0f, 0.5f);
	
//	mBFilterByEarDist.set("FilterByEarDist", true);
//	mMinEarDist.set("MinEarDistToKeep", 0.01f, 0.0f, 0.25f);
//	mMinEarDistToBeginTracking.set("MinEarDistToBegin", 0.05f, 0.0, 0.25f);
}

//--------------------------------------------------------------
PoseTracker::~PoseTracker() {
	Tracker::release();
}

//----------------------------------------------------------
ofParameterGroup& PoseTracker::getParams() {
	if( params.size() < 1 ) {
		params.setName(mGuiPrefix+"MediaPipePoseTracker");
		_addToParams();
		if( mSettings.outputSegmentationMasks ) {
			params.add( mBDrawMask );
		}
		params.add( mPosSmoothing );
		
//		params.add( mBFilterMatchingFaces );
//		params.add( mMatchFaceDistance );
		
//		params.add( mBFilterByEarDist );
//		params.add( mMinEarDist );
//		params.add( mMinEarDistToBeginTracking );
	}
	return params;
}

//--------------------------------------------------------------
bool PoseTracker::setup(const Tracker::Settings& asettings) {
	return setup( PoseSettings(asettings) );
}

//--------------------------------------------------------------
bool PoseTracker::setup(const PoseSettings& asettings) {
	if (!Py_IsInitialized()) {
		ofLogNotice("PoseTracker") << "initing PYTHON";
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
		mSettings.filePath = ofToDataPath("pose_landmarker_full.task", true);
		if( !ofFile::doesFileExist(mSettings.filePath) ) {
			// trying to load from addon folder
			mSettings.filePath = ofToDataPath("../../../../../addons/ofxMediaPipePython/tasks/pose/pose_landmarker_full.task", true);
		}
	}
	
	if( !ofFile::doesFileExist(mSettings.filePath) ) {
		ofLogError("PoseTracker :: setup") << "pose land marker task not found at " << mSettings.filePath;
		return false;
	}
	
	Tracker::release();
	mMaskPix.clear();
	mBExiting = false;
 	
	
	py::gil_scoped_acquire acquire;
	py_mediapipe = py::module::import("mediapipe");
	
	py::object BaseOptions = py_mediapipe.attr("tasks").attr("BaseOptions");
	py::object PoseLandmarker = py_mediapipe.attr("tasks").attr("vision").attr("PoseLandmarker");
	py::object PoseLandmarkerOptions = py_mediapipe.attr("tasks").attr("vision").attr("PoseLandmarkerOptions");
	py::object VisionRunningMode = py_mediapipe.attr("tasks").attr("vision").attr("RunningMode");

	// Create BaseOptions object
	py::object base_options = BaseOptions.attr("__call__")(py::arg("model_asset_path") = mSettings.filePath.string());
	
	
	//(py::object& aresults, py::object& aMpImage, int aTimestamp);
	process_results_lambda = [this](py::object& aresults, py::object& aMpImage, int aTimestamp) {
		//		std::cout << "Class Lambda Result callback called. " << aTimestamp << std::endl;
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
			ofLogNotice("Media pipe pose tracker") << " error with the callback function.";
		}
	};
	
	std::string rmodeStr = Tracker::sGetStringForRunningMode(mSettings.runningMode);
	
	py::object options;
	if( mSettings.runningMode == Tracker::MODE_LIVE_STREAM ) {
		options = PoseLandmarkerOptions.attr("__call__")(
														 py::arg("base_options") = base_options,
														 py::arg("running_mode") = VisionRunningMode.attr(rmodeStr.c_str()),
														 py::arg("num_poses") = mSettings.maxNum,
														 py::arg("min_pose_detection_confidence") = mSettings.minDetectionConfidence,
														 py::arg("min_tracking_confidence") = mSettings.minTrackingConfidence,
														 py::arg("min_pose_presence_confidence") = mSettings.minPresenceConfidence,
														 py::arg("output_segmentation_masks") = mSettings.outputSegmentationMasks ? 1 : 0,
														 py::arg("result_callback") = py::cpp_function(process_results_lambda)
														 );
	} else {
		options = PoseLandmarkerOptions.attr("__call__")(
														 py::arg("base_options") = base_options,
														 py::arg("running_mode") = VisionRunningMode.attr(rmodeStr.c_str()),
														 py::arg("num_poses") = mSettings.maxNum,
														 py::arg("min_pose_detection_confidence") = mSettings.minDetectionConfidence,
														 py::arg("min_tracking_confidence") = mSettings.minTrackingConfidence,
														 py::arg("min_pose_presence_confidence") = mSettings.minPresenceConfidence,
														 py::arg("output_segmentation_masks") = mSettings.outputSegmentationMasks ? 1 : 0
														 );
	}
	
	
	py_landmarker = PoseLandmarker.attr("create_from_options")(options);
//	sNumPyInstances++;
	_addAppListeners();
	
//	if( getRunningMode() == Tracker::MODE_OF_VIDEO_THREAD ) {
		py::gil_scoped_release release; // add this to release the GIL
//	}
	
	Tracker::sNumPyInstances++;
	
	mBSetup = true;
	return true;
}

//--------------------------------------------------------------
void PoseTracker::_update() {
	if( mSettings.runningMode == Tracker::MODE_LIVE_STREAM || mSettings.runningMode == Tracker::MODE_OF_VIDEO_THREAD ) {
		if(mHasNewThreadValues.load() ) {
			std::unique_lock<std::mutex> lck(mMutex);
//			mPoses = mThreadedPoses;
//			mRawPoses = mThreadedPoses;
			mRawPoses.clear();
			for( auto& tp : mThreadedPoses ) {
				auto np = std::make_shared<Pose>(*tp);
				np->updateFromKeypoints();
				mRawPoses.push_back(np);
			}
			
			_matchPoses(mThreadedPoses, mPoses);
			// we need to match the poses here //
//			mThreadedPoses.clear();
			mBTexDirty = true;
			// this probably isn't thread safe :/
			if( mSettings.outputSegmentationMasks && mPixWidth > 0 && mPixHeight > 0 ) {
				mMaskPix.setFromPixels(mPixelData.data(), mPixWidth, mPixHeight, 1 );
			}
			
			mHasNewThreadValues = false;
			mBHasNewData = true;
			mFpsCounter.newFrame();
		}
	}
	
//	float deltaTime = ofClamp( ofGetLastFrameTime(), 1.f/5.f, 1.f/5000.f);

	_calculateDeltatime();
	float deltaTime = mDeltaTimeSmoothed;
	
	for( auto& pose : mPoses ) {
		pose->age += deltaTime;
		
	}
}

//--------------------------------------------------------------
void PoseTracker::draw() {
	ofSetColor( ofColor::purple );
	for( auto& rpose : mRawPoses ) {
		rpose->drawOutlines(mBDrawUsePosZ);
	}
//	ofDrawCircle(20, 20, 10);
	
	ofSetColor( ofColor::magenta );
	for( auto& pose : mPoses ) {
		if( mBDrawPoints ) {
			pose->drawPoints(mDrawPointSize, mBDrawUsePosZ);
		}
		if( mBDrawOutlines ) {
//			ofSetColor(255);
			pose->drawOutlines(mBDrawUsePosZ);
		}
	}
	
	if( mBDrawMask ) {
		if( getMaskTexture().isAllocated() ) {
			getMaskTexture().draw( mOutRect );
		}
	}
}

//----------------------------------------------------------------------
void PoseTracker::draw(float x, float y, float w, float h) const {
	for( auto& pose : mPoses ) {
		pose->drawPoints(1, x, y, w, h);
//		pose->getKeypointsNormalizedLineMesh().draw();
	}
}

//--------------------------------------------------------------
ofFloatPixels& PoseTracker::getMaskPixels() {
	return mMaskPix;
}

//--------------------------------------------------------------
ofTexture& PoseTracker::getMaskTexture() {
	if( !mSettings.outputSegmentationMasks ) {
		ofLogError("PoseTracker::getMaskTexture") << "must call setup with outputSegmentationMasks=true";
		return mMaskTexture;
	}
	
	if( mMaskPix.getWidth() < 1 || mMaskPix.getHeight() < 1 ) {
		return mMaskTexture;
	}
	
	if( mBTexDirty ) {
		mBTexDirty = false;
//		ofLogNotice("PoseTracker::getMaskTexture") << "mBTexDirty";
		mMaskTexture.loadData(mMaskPix);
	}
	return mMaskTexture;
}

//pose_landmarks : [[NormalizedLandmark(x=0.3585308790206909, y=0.4075578451156616, z=-0.3978717625141144, visibility=0.9998270869255066, presence=0.998150646686554), NormalizedLandmark(x=0.39301782846450806, y=0.34413063526153564, z=-0.42004331946372986, visibility=0.9997126460075378, presence=0.9961573481559753), NormalizedLandmark(x=0.41013503074645996, y=0.3422584533691406, z=-0.41997912526130676, visibility=0.9997565150260925, presence=0.9958040118217468), NormalizedLandmark(x=0.4293026328086853, y=0.34123873710632324, z=-0.42015504837036133, visibility=0.9997486472129822, presence=0.9950726628303528), NormalizedLandmark(x=0.36083781719207764, y=0.3503305912017822, z=-0.34061363339424133, visibility=0.999631404876709, presence=0.9963182210922241), NormalizedLandmark(x=0.35542333126068115, y=0.3506617546081543, z=-0.34022048115730286, visibility=0.9996902942657471, presence=0.9965831637382507), NormalizedLandmark(x=0.3507296144962311, y=0.35108160972595215, z=-0.34086182713508606, visibility=0.9997046589851379, presence=0.9966642260551453), NormalizedLandmark(x=0.4851182699203491, y=0.38132381439208984, z=-0.3646034002304077, visibility=0.9997510313987732, presence=0.9953799247741699), NormalizedLandmark(x=0.379803866147995, y=0.3835180997848511, z=-0.009857963770627975, visibility=0.9996645450592041, presence=0.9978312849998474), NormalizedLandmark(x=0.38743072748184204, y=0.46527349948883057, z=-0.37417104840278625, visibility=0.9997569918632507, presence=0.9983274340629578), NormalizedLandmark(x=0.3531826138496399, y=0.4727611541748047, z=-0.2717677354812622, visibility=0.9996339082717896, presence=0.9976716637611389), NormalizedLandmark(x=0.6579610705375671, y=0.7327575087547302, z=-0.3604966700077057, visibility=0.9932035207748413, presence=0.970598578453064), NormalizedLandmark(x=0.3003303110599518, y=0.6942197680473328, z=0.08058322221040726, visibility=0.9984002709388733, presence=0.9955385327339172), NormalizedLandmark(x=0.666886031627655, y=1.0972154140472412, z=-0.4738471210002899, visibility=0.23380111157894135, presence=0.012096378952264786), NormalizedLandmark(x=0.11790674924850464, y=0.9390098452568054, z=-0.01799035258591175, visibility=0.3496042788028717, presence=0.13078776001930237), NormalizedLandmark(x=0.6156686544418335, y=1.4886183738708496, z=-0.5814182162284851, visibility=0.05010611191391945, presence=0.0028123212978243828), NormalizedLandmark(x=-0.07843983173370361, y=1.0552154779434204, z=-0.5770008563995361, visibility=0.30764248967170715, presence=0.03802492097020149), NormalizedLandmark(x=0.6219452619552612, y=1.6049809455871582, z=-0.6408757567405701, visibility=0.08116517961025238, presence=0.003354862332344055), NormalizedLandmark(x=-0.16131848096847534, y=1.1012294292449951, z=-0.6745758056640625, visibility=0.3546183109283447, presence=0.027353130280971527), NormalizedLandmark(x=0.5812628269195557, y=1.603255271911621, z=-0.6837242841720581, visibility=0.13708724081516266, presence=0.0067890663631260395), NormalizedLandmark(x=-0.13850826025009155, y=1.0989999771118164, z=-0.7527515292167664, visibility=0.5112929344177246, presence=0.056434016674757004), NormalizedLandmark(x=0.5711681246757507, y=1.553577184677124, z=-0.6063789129257202, visibility=0.12238670140504837, presence=0.005118418484926224), NormalizedLandmark(x=-0.07258814573287964, y=1.0547164678573608, z=-0.6417446732521057, visibility=0.5215182304382324, presence=0.05614463612437248), NormalizedLandmark(x=0.6000320315361023, y=1.4612900018692017, z=-0.11178039014339447, visibility=0.0012885683681815863, presence=0.00040999482735060155), NormalizedLandmark(x=0.3408632278442383, y=1.4409656524658203, z=0.11582030355930328, visibility=0.0021444896701723337, presence=0.0007206244044937193), NormalizedLandmark(x=0.5613062381744385, y=2.062425136566162, z=-0.28075259923934937, visibility=0.002317616483196616, presence=9.370282350573689e-05), NormalizedLandmark(x=0.3167926073074341, y=2.0278360843658447, z=-0.026920877397060394, visibility=0.0008721136837266386, presence=0.00013540750660467893), NormalizedLandmark(x=0.5634738206863403, y=2.620117664337158, z=0.07365921884775162, visibility=0.0001044022137648426, presence=1.8720855905485223e-06), NormalizedLandmark(x=0.3150092363357544, y=2.596559524536133, z=0.10626097768545151, visibility=1.5652485672035255e-05, presence=1.6978951862256508e-06), NormalizedLandmark(x=0.5777183771133423, y=2.7070446014404297, z=0.08166775107383728, visibility=7.910205749794841e-05, presence=1.3861879324394977e-06), NormalizedLandmark(x=0.31474941968917847, y=2.682009696960449, z=0.10073413699865341, visibility=2.8796170226996765e-05, presence=1.6307973282891908e-06), NormalizedLandmark(x=0.4953860640525818, y=2.771178722381592, z=-0.44110631942749023, visibility=0.00011405318946344778, presence=4.841573172598146e-06), NormalizedLandmark(x=0.3124222755432129, y=2.7602574825286865, z=-0.4420236349105835, visibility=3.443786408752203e-05, presence=6.3576276261301246e-06)]]
//pose_world_landmarks : [[Landmark(x=-0.09860840439796448, y=-0.5988280177116394, z=-0.13318729400634766, visibility=0.9998270869255066, presence=0.998150646686554), Landmark(x=-0.08913175761699677, y=-0.633057713508606, z=-0.14086055755615234, visibility=0.9997126460075378, presence=0.9961573481559753), Landmark(x=-0.08858872205018997, y=-0.6343879103660583, z=-0.1406266689300537, visibility=0.9997565150260925, presence=0.9958040118217468), Landmark(x=-0.08880133926868439, y=-0.634470522403717, z=-0.14112329483032227, visibility=0.9997486472129822, presence=0.9950726628303528), Landmark(x=-0.10397592931985855, y=-0.631391167640686, z=-0.11961889266967773, visibility=0.999631404876709, presence=0.9963182210922241), Landmark(x=-0.10425901412963867, y=-0.6324041485786438, z=-0.12152743339538574, visibility=0.9996902942657471, presence=0.9965831637382507), Landmark(x=-0.10478109866380692, y=-0.6331320405006409, z=-0.12047457695007324, visibility=0.9997046589851379, presence=0.9966642260551453), Landmark(x=0.001912541687488556, y=-0.6134018898010254, z=-0.12145638465881348, visibility=0.9997510313987732, presence=0.9953799247741699), Landmark(x=-0.07109137624502182, y=-0.6192240118980408, z=-0.03718376159667969, visibility=0.9996645450592041, presence=0.9978312849998474), Landmark(x=-0.059387482702732086, y=-0.5739741325378418, z=-0.1281578540802002, visibility=0.9997569918632507, presence=0.9983274340629578), Landmark(x=-0.08080808818340302, y=-0.5722774267196655, z=-0.10178685188293457, visibility=0.9996339082717896, presence=0.9976716637611389), Landmark(x=0.13900965452194214, y=-0.4294555187225342, z=-0.08351275324821472, visibility=0.9932035207748413, presence=0.970598578453064), Landmark(x=-0.1580343246459961, y=-0.45263275504112244, z=-0.007599592208862305, visibility=0.9984002709388733, presence=0.9955385327339172), Landmark(x=0.15069018304347992, y=-0.1939898133277893, z=-0.13025832176208496, visibility=0.23380111157894135, presence=0.012096378952264786), Landmark(x=-0.288615882396698, y=-0.29693353176116943, z=-0.055650241672992706, visibility=0.3496042788028717, presence=0.13078776001930237), Landmark(x=0.11333555728197098, y=-0.0411730632185936, z=-0.17223238945007324, visibility=0.05010611191391945, presence=0.0028123212978243828), Landmark(x=-0.4106437861919403, y=-0.23508697748184204, z=-0.19604885578155518, visibility=0.30764248967170715, presence=0.03802492097020149), Landmark(x=0.11019149422645569, y=0.02855795994400978, z=-0.17830514907836914, visibility=0.08116517961025238, presence=0.003354862332344055), Landmark(x=-0.4369336664676666, y=-0.20625077188014984, z=-0.2174210548400879, visibility=0.3546183109283447, presence=0.027353130280971527), Landmark(x=0.07376240193843842, y=0.014736926183104515, z=-0.19569134712219238, visibility=0.13708724081516266, presence=0.0067890663631260395), Landmark(x=-0.4252292513847351, y=-0.2152135968208313, z=-0.24047446250915527, visibility=0.5112929344177246, presence=0.056434016674757004), Landmark(x=0.09188370406627655, y=-0.029499327763915062, z=-0.17067193984985352, visibility=0.12238670140504837, presence=0.005118418484926224), Landmark(x=-0.40492159128189087, y=-0.2253665179014206, z=-0.209733247756958, visibility=0.5215182304382324, presence=0.05614463612437248), Landmark(x=0.13245484232902527, y=0.006715020164847374, z=-0.0022287368774414062, visibility=0.0012885683681815863, presence=0.00040999482735060155), Landmark(x=-0.13512767851352692, y=-0.03341776132583618, z=0.00670170783996582, visibility=0.0021444896701723337, presence=0.0007206244044937193), Landmark(x=0.11137822270393372, y=-0.0031447215005755424, z=-0.15964365005493164, visibility=0.002317616483196616, presence=9.370282350573689e-05), Landmark(x=-0.1231611967086792, y=-0.2577091455459595, z=-0.2089080810546875, visibility=0.0008721136837266386, presence=0.00013540750660467893), Landmark(x=0.1818404197692871, y=0.2298084795475006, z=-0.025336742401123047, visibility=0.0001044022137648426, presence=1.8720855905485223e-06), Landmark(x=-0.12735193967819214, y=0.03549639880657196, z=-0.19426536560058594, visibility=1.5652485672035255e-05, presence=1.6978951862256508e-06), Landmark(x=0.1754114031791687, y=0.2980237901210785, z=0.01984882354736328, visibility=7.910205749794841e-05, presence=1.3861879324394977e-06), Landmark(x=-0.13841398060321808, y=0.10805466771125793, z=-0.11989021301269531, visibility=2.8796170226996765e-05, presence=1.6307973282891908e-06), Landmark(x=0.2692042589187622, y=-0.13185535371303558, z=-0.3709244728088379, visibility=0.00011405318946344778, presence=4.841573172598146e-06), Landmark(x=-0.14211127161979675, y=-0.19881002604961395, z=-0.49884653091430664, visibility=3.443786408752203e-05, presence=6.3576276261301246e-06)]]
//segmentation_masks : [<mediapipe.python._framework_bindings.image.Image object at 0x2848aa6f0>]

//--------------------------------------------------------------
void PoseTracker::_process_landmark_results(py::object& aresults, int aTimestamp) {
	
//	ofLogNotice("_process_landmark_results") << getTrackerTypeAsString() << " | " << ofGetFrameNum();

	if (mBExiting.load()) {
		ofLogNotice("_process_landmark_results" ) << "EXITING: " << getTrackerTypeAsString() << " | " << ofGetFrameNum();
		return;
	}

	//	py::scoped_interpreter guard{};
	if (aresults.is_none()) {
		ofLogNotice("PoseTracker::_process_landmark_results") << "Results are bad, returning. " << aTimestamp;
		return;
	}
	
//	Utils::print_attributes(aresults);
//	return;

	bool bValid = true;

	std::vector< std::shared_ptr<Pose> > tposes;

	try {
//		py::gil_scoped_acquire acquire;
		if (!Utils::has_all_attributes(aresults, { "pose_landmarks", "pose_world_landmarks" })) {
			ofLogError("PoseTracker::_process_landmark_results") << "Does not contain either pose_landmarks or pose_world_landmarks." << aTimestamp;
			//py::gil_scoped_release release;
			return;
		}

		py::list pose_landmarks_list = aresults.attr("pose_landmarks").cast<py::list>();
		py::list pose_world_landmarks_list = aresults.attr("pose_world_landmarks").cast<py::list>();;
		//	Check if results is iterable
		if (!py::isinstance<py::iterable>(pose_landmarks_list)) {
			ofLogError("PoseTracker::_process_landmark_results") << "'results' pose_landmarks is not iterable. " << aTimestamp;
			//py::gil_scoped_release release;
			return;
		}
		
		
		

		//https://developers.google.com/mediapipe/solutions/vision/hand_landmarker/python#live-stream_2

		// Acquire GIL before interacting with Python objects
		//py::gil_scoped_acquire acquire;

		// std::vector< std::shared_ptr<Pose> > tposes;
		int numMarks = py::len(pose_landmarks_list);
		
//		ofLogNotice("_process_landmark_results") << " going to process number of marks: " << numMarks << " | " << ofGetFrameNum();
		
		for (int i = 0; i < numMarks; i++) {

			//		Pose tpose;
			auto tpose = std::make_shared<Pose>();

			py::list pose_landmarks = pose_landmarks_list[i];
			py::list pose_world_landmarks = pose_world_landmarks_list[i];

			int num = py::len(pose_landmarks);

			//		ofLogNotice("PoseTracker::_process_landmark_results") << " num keypoints " << num;

			tpose->keypoints.assign(num, TrackedObject::Keypoint());

			int j = 0;
			for (py::handle hl : pose_landmarks) {

				auto& kp = tpose->keypoints[j];
				kp.pos.x = py::float_(py::getattr(hl, "x"));
				kp.pos.y = py::float_(py::getattr(hl, "y"));
				kp.pos.z = py::float_(py::getattr(hl, "z"));

				kp.posN = kp.pos;
				kp.pos.x *= mOutRect.width;
				kp.pos.y *= mOutRect.height;
				kp.pos.x += mOutRect.x;
				kp.pos.y += mOutRect.y;

				kp.pos.z *= mOutRect.width;

				//			ofLogNotice("PoseTracker::_process_landmark_results") << j << " keypoint pos " << kp.pos;

				j++;
			}

			j = 0;
			for (py::handle hwl : pose_world_landmarks) {
				auto& kp = tpose->keypoints[j];
				kp.posWorld.x = py::float_(py::getattr(hwl, "x"));
				kp.posWorld.y = py::float_(py::getattr(hwl, "y"));
				kp.posWorld.z = py::float_(py::getattr(hwl, "z"));
				j++;
			}

			tposes.push_back(tpose);
		}

		if (mSettings.outputSegmentationMasks) {
			// update the mask
			// if (!py::isinstance<py::iterable>(pose_landmarks_list)) {
			if (Utils::has_attribute(aresults, "segmentation_masks") && py::isinstance<py::iterable>(aresults.attr("segmentation_masks"))) {
				py::list pose_segmentation_list = aresults.attr("segmentation_masks").cast<py::list>();
				int numMasks = py::len(pose_segmentation_list);
				if (numMasks > 0) {
					// https://developers.google.com/mediapipe/api/solutions/python/mp/Image
	//				py::object segMask = pose_segmentation_list[0];

					py::array_t<float> segResult = pose_segmentation_list[0].attr("numpy_view")().cast<py::array_t<float>>();
					//				Utils::print_attributes(segMask);
					// Access the shape attribute of the array
					py::tuple shape = segResult.attr("shape");

					// Extract the width and height from the shape tuple
					mPixHeight = shape[0].cast<int>();
					mPixWidth = shape[1].cast<int>();

					// Print the width and height
	//				std::cout << "Width: " << mPixWidth << std::endl;
	//				std::cout << "Height: " << mPixHeight << std::endl;


					auto buf = segResult.request();
					//				mMaskPix.setFromPixels(buf.ptr, width, height, 1 );
									// Create a vector to hold the data
					//				std::vector<uint8_t> pixVec(buf.size);
					if (mPixelData.size() != buf.size) {
						mPixelData.assign(buf.size, 0);
					}

					// Copy the data from the array to the vector
	//				std::memcpy(mPixelData.data(), (uint8_t*)buf.ptr, buf.size * sizeof(uint8_t));
					std::memcpy(mPixelData.data(), (float*)buf.ptr, buf.size * sizeof(float));

				}
			} else {
				ofLogError("PoseTracker::_process_landmark_results") << "Does not contain segmentation_masks." << aTimestamp;
			}
		}

		//py::gil_scoped_release release;
	} catch (...) {
		ofLogError("PoseTracker") << __FUNCTION__ << " error with gil";
		return;
	}

	// Release GIL while performing intensive C++ operations
	//py::gil_scoped_release release;

	if( mSettings.runningMode == Tracker::MODE_LIVE_STREAM) {
		std::lock_guard<std::mutex> lck(mMutex);
		mThreadedPoses = tposes;
		mHasNewThreadValues = true;
	} else if( mSettings.runningMode == Tracker::MODE_OF_VIDEO_THREAD ) {
		//std::unique_lock<std::mutex> lck(mMutex);
		mThreadedPoses = tposes;
		mHasNewThreadValues = true;
	} else {
		if( mSettings.outputSegmentationMasks && mPixWidth > 0 && mPixHeight > 0 ) {
			mMaskPix.setFromPixels(mPixelData.data(), mPixWidth, mPixHeight, 1 );
		}
		mBTexDirty = true;
		mRawPoses = tposes;
		_matchPoses( tposes, mPoses );
	}
}

//--------------------------------------------------------------
//void PoseTracker::_matchPoses( std::vector<Pose>& aIncomingPoses, std::vector< std::shared_ptr<Pose>>& aPoses ) {
void PoseTracker::_matchPoses( std::vector< std::shared_ptr<Pose>>& aIncomingPoses, std::vector< std::shared_ptr<Pose>>& aPoses ) {
	
	float frameRate = ofClamp(Tracker::mFpsCounter.getFps(), 1, 200);
	int mNumFramesToDie = Tracker::mMaxTimeToMatch * frameRate;

	for( auto& pose : aPoses ) {
		pose->trackingData.bFoundThisFrame = false;
		pose->trackingData.numFramesNotFound++;
		if( pose->trackingData.numFramesNotFound > mNumFramesToDie ) {
			pose->bRemove = true;
		}
	}
	
	ofRemove( aPoses, TrackedObject::shouldRemove );
	
	auto validIncomingPoses = aIncomingPoses;
	
	// lets try to filter out wonky poses
	// ie if poses have the same face, check which might be the better solution //
//	if( mBFilterMatchingFaces && aIncomingPoses.size() > 1 ) {
//		validIncomingPoses.clear();
//		float maxFaceDist = mMatchFaceDistance;
//		
//		std::vector<bool> poseValidities( aIncomingPoses.size(), true );
//		
//		for( std::size_t i = 0; i < aIncomingPoses.size()-1; i++ ) {
//			if(!poseValidities[i]) {
//				// we have already determined this pose to be invalid //
//				continue;
//			}
//			
//			auto cpose = aIncomingPoses[i];
//			bool bPoseValid = true;
//			for( std::size_t j = i+1; j < aIncomingPoses.size(); j++ ) {
//				if(!poseValidities[j]) {
//					// already determined to be invalid
//					continue;
//				}
//				auto opose = aIncomingPoses[j];
//				if( cpose.get() == opose.get() ) {
//					continue; // just in case
//				}
//				float faceDist = glm::distance(cpose->getFacePositionNormalized(), opose->getFacePositionNormalized());
//				if( faceDist < maxFaceDist ) {
//					
//					// we need to choose one, so make sure if we can't pick the best, choose one //
//					bool bFiltered = false;
//					// ok, we potentially have a conflict. Lets try to select the best one //
//					bool cup = _isFaceDirectionUp( cpose );
//					bool oup = _isFaceDirectionUp( opose );
//					if( cup != oup ) {
//						if( cup && !oup ) {
//							// other pose is not valid
//							// we want to have the face direction up
//							poseValidities[j] = false;
//						} else {
//							// our current pose is not valid, carry on
//							poseValidities[i] = false;
//							continue;
//						}
//						bFiltered = true;
//					}
//					
//					// are our feet above our head?
//					bool cfeet = _areFeetAboveHead(cpose);
//					bool ofeet = _areFeetAboveHead(opose);
//					if( cfeet != ofeet) {
//						if( cfeet ) {
//							// our current is invalid since feet are above head
//							poseValidities[i] = false;
//							continue;
//						} else {
//							// the other pose has feet above head
//							poseValidities[j] = false;
//						}
//						bFiltered = true;
//					}
//					
//					if( !bFiltered ) {
//						
//						int betterIndex = _hasBetterMatchToExistingPose( cpose, opose, aPoses );
//						if( betterIndex < 1 ) {
//							// the current pose has a better match
//							// invalidate the other
//							poseValidities[j] = false;
//						} else {
//							// the other pose has a better match
//							poseValidities[i] = false;
//						}
//						
//						// we just have to choose one, so hopefully it's the first one
//						// it should be better than having two overlapping poses
////						poseValidities[j] = false;
//					}
//					
//				}
//			}
//			
//			if( poseValidities[i] ) {
//				validIncomingPoses.push_back(cpose);
//			}
//		}
//		
//		
////		ofLogNotice( "MediaPipePoseTracker" ) << "_matchPoses :: num incoming poses: " << aIncomingPoses.size() << " num valid: " << validIncomingPoses.size() << " | " << ofGetFrameNum();
//	}
	
//	float earDistToKeepTracking = mMinEarDist * mSrcRect.getWidth();
//	float earDistToStartTracking = mMinEarDistToBeginTracking * mSrcRect.getWidth();
//	if( earDistToKeepTracking > earDistToStartTracking ) {
//		earDistToKeepTracking = earDistToStartTracking;
//	}
	
//	// first lets see if the poses have met the continued ear distance
//	if( mBFilterByEarDist ) {
//		for( auto& pose : aPoses ) {
//			auto p1 = pose->getKeypointForIndex(Pose::RIGHT_EAR).pos;
//			auto p2 = pose->getKeypointForIndex(Pose::LEFT_EAR).pos;
//			float earDist = glm::length(p2-p1);
//			
//			ofLogNotice("PoseTracker::_matchPoses") << "pose: " << pose->ID << " right ear: " << p1 << " left ear: " << p2 << " dist: " << earDist << " | " << ofGetFrameNum();
//			
//			if( earDist < earDistToKeepTracking ) {
//				continue;
//			}
//		}
//	}
	
	
	float maxDistToMatch2 = mMaxDistToMatch * mMaxDistToMatch;
	//	glm::vec3 posN = {0.f, 0.f, 0.f };
	// now lets try to find a hand to match //
	for( auto& aInPose : validIncomingPoses ) {
		// try to match with current hands
		std::shared_ptr<Pose> bestMatch;
		float closestDistMatch = 9999. * 8874.f;
		for( auto& pose : aPoses ) {
			pose->trackingData.matchDistance = 0.f;
			if( pose->trackingData.bFoundThisFrame ) {continue;}
			// simple distance calculation //
			// to determine if within realm of match
//			float dist2 = glm::distance2( pose->getPositionNormalized(), aInPose->getPositionNormalized() );
			float dist = glm::distance( pose->getPositionNormalized(), aInPose->getPositionNormalized() );
			if( dist < mMaxDistToMatch ) {
				
				// filter through the ear distance to continue tracking //
				// this value should be smaller than the begin tracking value
//				if( mBFilterByEarDist ) {
//					auto p1 = pose->getKeypointForIndex(Pose::RIGHT_EAR).pos;
//					auto p2 = pose->getKeypointForIndex(Pose::LEFT_EAR).pos;
//					float earDist = glm::length(p2-p1);
//					
//					ofLogNotice("PoseTracker::_matchPoses") << "pose: " << pose->ID << " right ear: " << p1 << " left ear: " << p2 << " dist: " << earDist << " | " << ofGetFrameNum();
//					
//					if( earDist < earDistToKeepTracking ) {
//						continue;
//					}
//				}
				
				// possible hand match //
				pose->trackingData.matchDistance += dist;
				
				// figure out some more confidence match here
				if( dist < closestDistMatch ) {
					bestMatch = pose;
					closestDistMatch = dist;//pose->trackingData.matchDistance;
				}
			}
		}
		
		shared_ptr<Pose> myPose;
		if( bestMatch ) {
			bestMatch->trackingData.bFoundThisFrame = true;
			bestMatch->trackingData.numFramesNotFound = 0;
//			bestMatch->keypoints = tpose->keypoints;
			myPose = bestMatch;
		} else {
			bool bYeahMakeOne = true;
//			if( mBFilterByEarDist ) {
//				auto p1 = aInPose->getPosForIndex(Pose::RIGHT_EAR);
//				auto p2 = aInPose->getPosForIndex(Pose::LEFT_EAR);
//				float earDist = glm::length(p2-p1);
//				if( earDist < earDistToStartTracking ) {
//					bYeahMakeOne = false;
//				}
//			}
			if(bYeahMakeOne) {
				// we need to add a pose //
				myPose = std::make_shared<Pose>(*(aInPose.get()));
				myPose->ID = (mCounterId++);
				aPoses.push_back(myPose);
			}
		}
		
		if( myPose ) {
//			myPose->updateFromKeypoints();
			if( mPosSmoothing < 0.001f || mPosSmoothing > 0.99f ){
				myPose->updateFrom(aInPose);
			} else {
				myPose->updateFromPoseWithSmoothing(aInPose, mPosSmoothing);
			}
		}
	}
}

//--------------------------------------------------------------
bool PoseTracker::_areFeetAboveHips( std::shared_ptr<Pose>& apose ) {
	if( !apose ) { return false; }
	auto hipPos = apose->getHipsMidPoint(0.f);
	auto rFootPos = apose->getKeypointForIndex(Pose::RIGHT_FOOT_INDEX).pos;
	auto lFootPos = apose->getKeypointForIndex(Pose::LEFT_FOOT_INDEX).pos;
	
	return (lFootPos.y < hipPos.y) && (rFootPos.y < hipPos.y);
}

//--------------------------------------------------------------
bool PoseTracker::_areFeetAboveHead( std::shared_ptr<Pose>& apose ) {
	auto headPos = apose->getFacePosition(0.f);
	auto rFootPos = apose->getKeypointForIndex(Pose::RIGHT_FOOT_INDEX).pos;
	auto lFootPos = apose->getKeypointForIndex(Pose::LEFT_FOOT_INDEX).pos;
	
	return (lFootPos.y < headPos.y) && (rFootPos.y < headPos.y);
}

//--------------------------------------------------------------
bool PoseTracker::_isFaceDirectionUp( std::shared_ptr<Pose>& apose ) {
	auto faceDir = apose->getFaceDirectionScreen(1);
	return glm::dot( faceDir, glm::vec3(0.0f, -1.f, 0.0f) ) > 0.0f;
}

//--------------------------------------------------------------
float PoseTracker::_getMatchScore( std::shared_ptr<Pose>& aInPose, std::shared_ptr<Pose>& apose ) {
	int numKps = std::min(aInPose->keypoints.size(), apose->keypoints.size());
	if( numKps < 3 ) return 999999.f;
	
	float matchScore = 0.0f;
	for( int i = 0; i < numKps; i++ ) {
		matchScore += glm::distance2(aInPose->getKeypointForIndex(i).pos, apose->getKeypointForIndex(i).pos);
	}
	if( matchScore > 0.f ) {
		return sqrtf(matchScore);
	}
	
	return 999999.f;
}

//--------------------------------------------------------------
int PoseTracker::_hasBetterMatchToExistingPose( std::shared_ptr<Pose>& aInPose1, std::shared_ptr<Pose>& aInPose2, std::vector< std::shared_ptr<Pose>>& aPoses ) {
	if( aPoses.size() < 1 ) {
		// nothing to check against
		return 0;
	}
	float bestScore1 = 99999.f;
	for( auto& pose : aPoses ) {
		float score = _getMatchScore(aInPose1, pose);
		if( score < bestScore1 ) {
			bestScore1 = score;
		}
	}
	
	float bestScore2 = 99999.f;
	for( auto& pose : aPoses ) {
		float score = _getMatchScore(aInPose2, pose);
		if( score < bestScore2 ) {
			bestScore2 = score;
		}
	}
	
	if( bestScore2 < bestScore1 ) {
		return 1;
	}
	return 0;
}

#endif


