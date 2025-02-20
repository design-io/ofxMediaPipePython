# All variables and this file are optional, if they are not present the PG and the
# makefiles will try to parse the correct values from the file system.
#
# Variables that specify exclusions can use % as a wildcard to specify that anything in
# that position will match. A partial path can also be specified to, for example, exclude
# a whole folder from the parsed paths from the file system
#
# Variables can be specified using = or +=
# = will clear the contents of that variable both specified from the file or the ones parsed
# from the file system
# += will add the values to the previous ones in the file or the ones parsed from the file 
# system
# 
# The PG can be used to detect errors in this file, just create a new project with this addon 
# and the PG will write to the console the kind of error and in which line it is

meta:
	ADDON_NAME = ofxMediaPipePython
	ADDON_DESCRIPTION = openFramworks addon for Googles Mediapipe with python11 bindings
	ADDON_AUTHOR = @design-io
	ADDON_TAGS = "addon" "ofx" "mediapipe" "machine learning"
	ADDON_URL = https://developers.google.com/mediapipe/framework/getting_started/install

common:
	# dependencies with other addons, a list of them separated by spaces 
	# or use += in several lines
	# ADDON_DEPENDENCIES =
	
	# any special flag that should be passed to the linker when using this
	# addon, also used for system libraries with -lname
	# ADDON_LDFLAGS =
	
	# include search paths, this will be usually parsed from the file system
	# but if the addon or addon libraries need special search paths they can be
	# specified here separated by spaces or one per line using +=
	
	ADDON_INCLUDES += libs/pybind11/include/
	ADDON_INCLUDES += libs/python/include/python3.11/
	
	ADDON_DEFINES = OF_ADDON_HAS_OFX_MEDIAPIPE_PYTHON
	
osx:
	ADDON_LDFLAGS=-Wl,-rpath,@executable_path/ ../../../addons/ofxMediaPipePython/libs/python/lib/osx/libpython3.11.dylib
	
linux64:
	ADDON_LDFLAGS=-Wl,-rpath=./libs -Wl,-rpath=./
