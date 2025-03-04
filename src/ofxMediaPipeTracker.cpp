//
//  ofxMediaPipeTracker.cpp
//  HandShapeController
//
//  Created by Nick Hardeman on 4/25/24.
//

#include "ofxMediaPipeTracker.h"

#if !defined(OFX_MEDIAPIPE_EXCLUDE_TRACKERS)

using namespace ofx::MediaPipe;

using std::cout;
using std::endl;
using std::vector;

int Tracker::sNumPyInstances = 0;

//static bool sBPyInterpreterInited = false;

//--------------------------------------------------------------
bool Tracker::PyShutdown() {
	if (Py_IsInitialized()) {
		ofLogNotice("ofApp") << "finalize pybind interpreter.";
		try {
			
			try {
//				py::gil_scoped_acquire acquire;
				py::finalize_interpreter();
				return true;
			} catch (py::error_already_set& e) {
				std::cerr << "Python error: " << e.what() << std::endl;
			}
		} catch (const std::exception& e) {
			ofLogError("ERROR with scoped acquire") << e.what();
		} catch (...) {
			ofLogError("ERROR with scoped acquire");
		}
	}
	return false;
}

//--------------------------------------------------------------
void Tracker::release() {
	
	if( mVideoThreadRunning.load() ) {
		_stopVideoPixThread();
	}
	
	{
		ofLogNotice(getTrackerTypeAsString()) << " release method - pre lock";
		std::lock_guard<std::mutex> lck(mMutexMediaPipe);
		mBExiting = true;
		mBHasNewData = false;
		_removeAppListeners();
	}
	
	
//	std::lock_guard<std::mutex> lck(mMutexMediaPipe);
	// wait for thread to finish
//	if( !mBMediaPipeThreadFinished.load() ) {
	if(mThreadCallCount.load() > 0 ) {
		//int numTries = 0;
		for( int i = 0; i < 250; i++ ) {
//			if( mBMediaPipeThreadFinished.load() ) {
			if( mThreadCallCount.load() > 0 ) {
				ofLogNotice("ofxMediaPipeTracker :: release") << getTrackerTypeAsString() << " num threads: " << mThreadCallCount.load();
//				ofLogNotice(getTrackerTypeAsString()) << "release : media pipe thread has finished - " << i << " | " << ofGetFrameNum();
				break;
			}
//			ofLogNotice(getTrackerTypeAsString()) << "release : waiting for media pipe thread to finish - " << i << " | " << ofGetFrameNum();
			ofSleepMillis(2);
		}
	}
	mBMediaPipeThreadFinished = true;
	//mThreadCallCount = 0;

	//if( mBSetup ) {
		ofLogNotice(getTrackerTypeAsString()) << " release method - 2";
		//}

		if (process_results_lambda != nullptr) {
			process_results_lambda = nullptr;
		}

//	if( mBSetup ) {
//		sNumPyInstances--;
//		
//		if (Py_IsInitialized() && sNumPyInstances <= 0) {
////			ofLogNotice(getTrackerTypeAsString()) << " py::finalize_interpreter()";
////			py::finalize_interpreter();
//		}
//		if( sNumPyInstances < 0 ) {
//			sNumPyInstances = 0;
//		}
//	}
	
	
	if (mBSetup) {
		py::gil_scoped_acquire acquire;
		if(!py_landmarker.is_none() ) {
			ofLogNotice(getTrackerTypeAsString()) << ("py_landmarker release");
			try {
				// Acquire GIL before interacting with Python objects
				//			py::gil_scoped_acquire acquire;
				//py_landmarker.release();
				//py_landmarker = py::none();
				if (py::hasattr(py_landmarker, "close")) {
					ofLogNotice(getTrackerTypeAsString()) << ("py_landmarker close");
					py_landmarker.attr("close")();
				}
				//py_landmarker = py::none();
				//py::gil_scoped_release release;
			} catch (const std::exception& e) {
				ofLogNotice(getTrackerTypeAsString()) << ("py_landmarker close ERROR");
			} catch (...) {
				ofLogNotice(getTrackerTypeAsString()) << ("py_landmarker close ERROR");
			}
		}
		py::gil_scoped_release release;
		
	}
	
	
	
	if( mBSetup ) {
		mBSetup = false;
		sNumPyInstances--;
		
		if( sNumPyInstances <= 0 ) {
//			if (Py_IsInitialized()) {
//				Tracker::PyShutdown();
//			}
		}
		
		if( sNumPyInstances < 0 ) {
			sNumPyInstances = 0;
		}
	}
	//if (!py_mediapipe.is_none()) {
		//ofLogNotice(getTrackerTypeAsString()) << ("py_mediapipe release");
		//py_mediapipe.release();
		//py_mediapipe = py::none();
	//}
	
//	mBSetup = false;
	//if( mBSetup ) {
		ofLogNotice(getTrackerTypeAsString()) << " end of release method : setup " << mBSetup;
	//}
	
	mBSetup = false;
}

//--------------------------------------------------------------
void Tracker::update(ofEventArgs& args) {
	mBHasNewData = false;
	if(mCounterId > 99999) {
		mCounterId = 1;
	}
	if(mBExiting.load()) {
		return;
	}
	_update();
}

//----------------------------------------------------------------------
void Tracker::_calculateDeltatime(){
	float tnow = ofGetElapsedTimef();
	if( mLastTimeUpdateF < 0 ){
		mLastTimeUpdateF = tnow-(1.0/30.0);
		mDeltaTimeSmoothed = 1.0/30.0;
	}
	
	float tDeltaTime = ofClamp( tnow-mLastTimeUpdateF, 1.0/200.0, 1.0 );
	mDeltaTimeSmoothed = ofLerp(mDeltaTimeSmoothed, tDeltaTime, 0.5);
	mLastTimeUpdateF = tnow; 
}

//----------------------------------------------------------------------
void Tracker::process(const ofPixels& apix) {
//	if( getRunningMode() == Tracker::MODE_OF_VIDEO_THREAD ) {
		py::gil_scoped_release release; // add this to release the GIL
//	}
	
	if(!mBSetup) {
		ofLogWarning(getTrackerTypeAsString()) << " process: has not been setup properly.";
		return;
	}
	int pw = apix.getWidth();
	int ph = apix.getHeight();
	
	if( !apix.isAllocated() || pw < 10 || ph < 10 ) {
		ofLogWarning(getTrackerTypeAsString()) << " pixels are not allocated.";
		return;
	}
	
	mSrcRect.width = pw;
	mSrcRect.height = ph;
	
	if( mOutRect.width < 1 ) {
		mOutRect.width = pw;
	}
	if( mOutRect.height < 1 ) {
		mOutRect.height = ph;
	}
	
	_process_image( apix );
}

//-------------------------------------------------------------
void Tracker::_onExit( ofEventArgs& args ) {
//	release();
	_removeAppListeners();
	{
		std::lock_guard<std::mutex> lck(mMutexMediaPipe);
		mBExiting = true;
	}
}

//-------------------------------------------------------------
void Tracker::_process_image(const ofPixels& apix) {
	int pw = apix.getWidth();
	int ph = apix.getHeight();
	
	auto timestamp = (int)ofGetElapsedTimeMillis();

	if (mBExiting.load()) {
		return;
	}

	
	if( apix.getNumChannels() != 3 ) {
		ofLogWarning("Tracker::_process_image") << "pixels must have 3 channels, not " << apix.getNumChannels();
		return;
	}
	
	if( getRunningMode() == MODE_OF_VIDEO_THREAD ) {
		if( !mVideoThreadRunning.load() ) {
			_startVideoPixThread();
		}
	}


	
	if( getRunningMode() == Tracker::MODE_LIVE_STREAM ) {
		
		//		ofLogNotice("Tracker::_process_image") << "timestamp: " << timestamp << " | " << ofGetFrameNum();
		// if (mBMediaPipeThreadFinished.load()) {
		if (mThreadCallCount.load() < 1) {
			try {
				
				{
					std::lock_guard<std::mutex> lck(mMutexMediaPipe);
					mBMediaPipeThreadFinished = false;
					mThreadCallCount = mThreadCallCount + 1;
				}
				//			ofLogNotice("ofxMediaPipeTracker :: _process_image") << getTrackerTypeAsString() << " num threads: " << mThreadCallCount;
				// Acquire GIL before interacting with Python objects 
				//py::gil_scoped_acquire acquire;
				try {
					// Acquire GIL before interacting with Python objects 
					py::gil_scoped_acquire acquire;
					py::object mp_image = _getMpImageFromPixels(apix);
					//py_landmarker.attr("detect_async")(mp_image, timestamp);
					try {
						//py_landmarker.attr("detect_async")(mp_image, timestamp);
						py::function detect_async_fn = py_landmarker.attr("detect_async");
						if (!detect_async_fn.is_none()) {
							detect_async_fn(mp_image, timestamp);
						}
					} catch (py::error_already_set& e) {
						std::cerr << "Python error: " << e.what() << std::endl;
					}
					// Release GIL while performing intensive C++ operations
					//py::gil_scoped_release release;
				} catch (const std::exception& e) {

				} catch (...) {

				}

			} catch (...) {
				// Handle Python exception
				std::cerr << "Python exception in process_image:\n" << std::endl;// << e.what() << std::endl;
				return;// {}; // Return empty vector on error
			}
		}
		//		py_landmarker.attr("detect_async")(mp_image, timestamp);
		// landmarks gets set in the _update function per class, since it's thread specific 
	} else if( getRunningMode() == Tracker::MODE_VIDEO ) {
		py::gil_scoped_acquire acquire;
		py::object results;// = py_landmarker.attr("detect")(mp_image);
		py::object mp_image = _getMpImageFromPixels(apix);
		try {
			if (!mp_image.is_none()) {
				py::function detect_fn = py_landmarker.attr("detect_for_video");
				if( !detect_fn.is_none() ) {
					results = detect_fn(mp_image, timestamp);
				}
				//				results = py_landmarker.attr("detect_for_video")(mp_image, timestamp);
			}
		} catch (const py::error_already_set& e) {
			// Handle Python exception
			std::cerr << "Python exception in process_image:\n" << e.what() << std::endl;
			return;// {}; // Return empty vector on error
		} catch(...) {
			
		}
		// setup the landmarks
		_process_landmark_results(results, timestamp);
		mBHasNewData = true;
		mFpsCounter.newFrame();
	} else if( getRunningMode() == Tracker::MODE_OF_VIDEO_THREAD ) {
		
//		ofLogNotice( "Sending pixels" ) << " | " << ofGetFrameNum();
		if( !mBNewVideoPixels.load() ) {
//			py::gil_scoped_release release; // add this to release the GIL
//			std::lock_guard<std::mutex> lock(mVideoPixMutex);
			
			if( mThreadVideoPixels[mVideoPixIndex].getWidth() != apix.getWidth() ) {
				ofLogNotice("Tracker") << getTrackerTypeAsString() << " allocating pixels | " << ofGetFrameNum();
			}
			
//			auto cmillis =
			mThreadVideoPixels[mVideoPixIndex] = apix;
			std::lock_guard<std::mutex> lock(mVideoPixMutex);
			mBNewVideoPixels = true;
		}
	} else {
		py::gil_scoped_acquire acquire;
		py::object results;// = py_landmarker.attr("detect")(mp_image);
		py::object mp_image = _getMpImageFromPixels(apix);
		try {
			if (!mp_image.is_none()) {
				results = py_landmarker.attr("detect")(mp_image);
			}
		} catch (const py::error_already_set& e) {
			// Handle Python exception
			std::cerr << "Python exception in process_image:\n" << e.what() << std::endl;
			return;// {}; // Return empty vector on error
		}
		
		// setup the landmarks
		_process_landmark_results(results, timestamp);
		mBHasNewData = true;
		mFpsCounter.newFrame();
	}
}

//-------------------------------------------
void Tracker::_process_results_callback(py::object& aresults, py::object& aMpImage, int aTimestamp ) {
	
	if(mBExiting.load()) {
		std::lock_guard<std::mutex> lck(mMutexMediaPipe);
		mBMediaPipeThreadFinished = true;
		mThreadCallCount--;
		ofLogNotice("Tracker::_process_results_callback ! EXITING, returning") << " | " << ofGetFrameNum();
		return;
	}
	
	if( !mBSetup ) {
		std::lock_guard<std::mutex> lck(mMutexMediaPipe);
		mBMediaPipeThreadFinished = true;
		mThreadCallCount = 0;
		ofLogNotice("Tracker::_process_results_callback ! setup, returning") << " | " << ofGetFrameNum();
		return;
	}
//	ofLogNotice("Tracker::_process_landmark_results") << "timestamp: " << aTimestamp << " | " << ofGetFrameNum();
	{
		py::gil_scoped_acquire acquire;
		_process_landmark_results(aresults, aTimestamp);
	}
	
	
	std::lock_guard<std::mutex> lck(mMutexMediaPipe);
	mBMediaPipeThreadFinished = true;
	mThreadCallCount--;
	if( mThreadCallCount < 0 ) {
		mThreadCallCount = 0;
	}
//	ofLogNotice("Tracker::_process_results_callback") << "timestamp: " << aTimestamp << " finished: " << mBMediaPipeThreadFinished << " | " << ofGetFrameNum();
}

//--------------------------------------------------------------
void Tracker::_initParams() {
	mBDrawUsePosZ.set("DrawUsePosZ", false);
	mBDrawPoints.set("DrawPoints", false);
	mBDrawOutlines.set("DrawOutlines", true);
	mMaxDistToMatch.set("MaxDistToMatch", 0.25f, 0.0, 1.f);
	mMaxTimeToMatch.set("MaxTimeToMatch", 0.4f, 0.0, 5.0f);
}

//--------------------------------------------------------------
void Tracker::_addToParams() {
	params.add( mBDrawUsePosZ );
	params.add( mBDrawPoints );
	params.add( mBDrawOutlines );
	params.add( mMaxDistToMatch );
	params.add( mMaxTimeToMatch );
}

//--------------------------------------------------------------
void Tracker::_addAppListeners() {
	if( !mBHasAppListeners ) {
		ofAddListener(ofEvents().update, this, &Tracker::update );
		ofAddListener(ofEvents().exit, this, &Tracker::_onExit, OF_EVENT_ORDER_BEFORE_APP );
	}
	mBHasAppListeners=true;
}

//------------------------------------------------------------------------
void Tracker::_removeAppListeners() {
	if(mBHasAppListeners) {
		ofRemoveListener(ofEvents().update, this, &Tracker::update );
		ofRemoveListener(ofEvents().exit, this, &Tracker::_onExit, OF_EVENT_ORDER_BEFORE_APP );
	}
	mBHasAppListeners=false;
}

//------------------------------------------------------------------------
py::object Tracker::_getMpImageFromPixels(const ofPixels &apix) {
	std::string imgFmtStr = "SRGB";
	if( apix.getNumChannels() == 1 ) {
		imgFmtStr = "GRAY8";
	}
	
	int pw = apix.getWidth();
	int ph = apix.getHeight();
	
	int numChannels = apix.getNumChannels();
	
	py::object mp_image;
	bool bImageSet = false;
	
	try {
		// Acquire GIL before interacting with Python objects
//		py::gil_scoped_acquire acquire;
//		PyGILState_STATE gstate;
//		gstate = PyGILState_Ensure();  // Acquire GIL
		
		// Call Python API or functions here
		
		
//		if(try_acquire_gil_with_timeout(1000)) {
			// Create a NumPy array from the buffer
			py::array_t<uint8_t> image_array({ ph, pw, numChannels }, apix.getData());
			//	mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=cvimage)
			
//			py::module py_mediapipe = py::module::import("mediapipe");
		if( !py_mediapipe ) {
			ofLogNotice("Trying to grab media pipe");
			py_mediapipe = py::module::import("mediapipe");
		}
			py::object ImageFormat = py_mediapipe.attr("ImageFormat");
			py::object Image = py_mediapipe.attr("Image");
			
			
			// this should make a copy of the data
			mp_image = Image.attr("__call__")(
											  py::arg("image_format") = ImageFormat.attr(imgFmtStr.c_str()),
											  py::arg("data") = image_array
											  );
//			PyGILState_Release(gstate);  // Release GIL
			
			
			// Release GIL while performing intensive C++ operations
			//py::gil_scoped_release release;
			bImageSet = true;
//		}
	} catch (const std::exception& e) {
		
	} catch (...) {
		
	}
	return mp_image;
}

//------------------------------------------------------------------------
void Tracker::_startVideoPixThread() {
	if( !mVideoThreadRunning.load() ) {
		std::lock_guard<std::mutex> lock(mVideoPixMutex);
		mVideoPixIndex = 0;
		mVideoThreadRunning = true;
		mBNewVideoPixels = false;
		mBVideoThreadDone = false;
		mVideoThread = std::thread(&Tracker::_videoPixThreadedFunction, this );  // 'this' is passed to bind the object
		mVideoThread.detach();
	}
}

//------------------------------------------------------------------------
void Tracker::_stopVideoPixThread() {
	ofLogNotice("Stopping video pix thread.");
	mVideoThreadRunning = false;
	if( mVideoThread.joinable() ) {
		ofLogNotice("joining thread.");
		mVideoThread.join();
	} else {
		ofLogNotice("going to wait for thread.");
		// Wait for "joinWaitMillis" milliseconds for thread to finish
		std::unique_lock<std::mutex> lck(mVideoPixMutex);
		auto millis = 5000;
		if(!mBVideoThreadDone && mVideoThreadCondition.wait_for(lck,std::chrono::milliseconds(millis))==std::cv_status::timeout){
			// unable to completely wait for thread
			ofLogNotice("unable to completely wait for thread.");
		}
	}
}

//------------------------------------------------------------------------
void Tracker::_videoPixThreadedFunction() {
	while( mVideoThreadRunning.load() ) {
//		ofLogNotice("Video threaded function");
		if( mBNewVideoPixels.load() ) {
			py::gil_scoped_acquire acquire;
			py::object mp_image = _getMpImageFromPixels(mThreadVideoPixels[mVideoPixIndex.load()]);
			
			py::object results;
			auto timestamp = (int)ofGetElapsedTimeMillis();
			try {
				if (mp_image && py_landmarker ) {
					py::function detect_fn = py_landmarker.attr("detect_for_video");
					if( !detect_fn.is_none() ) {
						results = detect_fn(mp_image, timestamp);
					}
					
					if( results ) {
						//				ofLogNotice("Video thread function");
						_process_landmark_results(results, timestamp);
						
					} else {
						ofLogNotice("Video thread function") << "no results";
					}
				}
			} catch (const py::error_already_set& e) {
				// Handle Python exception
				std::cerr << "Python exception in process_image:\n" << e.what() << std::endl;
				return;// {}; // Return empty vector on error
			} catch(...) {
				
			}
			
			ofLogNotice("Thread") << getTrackerTypeAsString() << " time to process results: " << (ofGetElapsedTimeMillis()-timestamp);
			
			{
				std::lock_guard<std::mutex> lock(mVideoPixMutex);
				mVideoPixIndex = mVideoPixIndex ^ 1;
				mBNewVideoPixels = false;
//				mBHasNewData = true;
//				mFpsCounter.newFrame();
			}
			mVideoThreadCondition.notify_one();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	
	ofLogNotice("_videoPixThreadedFunction : no longer running");
	
	{
		std::lock_guard<std::mutex> lock(mVideoPixMutex);
		mBVideoThreadDone = true;
		mVideoThreadCondition.notify_all();
	}
}

#endif
