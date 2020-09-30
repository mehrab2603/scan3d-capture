# Based on https://github.com/VLOGroup/imageutilities/blob/master/cmake/FindFlyCapture2.cmake
# - Find Spinnaker
# This module finds if the Spinnaker SDK inlcudes and libraries are installed
#
# This module sets the following variables:
#
# SPINNAKER_FOUND
#    True if the Spinnaker SDK includes and libraries were found
# SPINNAKER_INCLUDE_DIRS
#    The include path of the Spinnaker.h header file
# SPINNAKER_LIBRARIES
#    The location of the library files

# check for 32 or 64bit
if(NOT WIN32 AND NOT APPLE)
  EXEC_PROGRAM(uname ARGS -m OUTPUT_VARIABLE CMAKE_CUR_PLATFORM)
  if( CMAKE_CUR_PLATFORM MATCHES "x86_64")
    set( HAVE_64_BIT 1 )
  else()
    set( HAVE_64_BIT 0 )
  endif()
else()
  if(CMAKE_CL_64)
    set( HAVE_64_BIT 1 )
  else()
    set( HAVE_64_BIT 0 )
  endif()
endif()

# set possible library paths depending on the platform architecture.
if(HAVE_64_BIT)
  set(CMAKE_LIB_ARCH_APPENDIX 64)
  set(SPINNAKER_POSSIBLE_LIB_DIRS "lib64" "lib" "bin" "/usr/lib" "/usr/local/lib" "vs2015" "lib64/vs2015" "lib/vs2015")
  #message( STATUS "FOUND 64 BIT SYSTEM")
else()
  set(CMAKE_LIB_ARCH_APPENDIX 32)
  set(SPINNAKER_POSSIBLE_LIB_DIRS "lib" "bin" "/usr/lib" "/usr/local/lib" "vs2015" "lib/vs2015")
  #message( STATUS "FOUND 32 BIT SYSTEM")
endif()

# FIND THE Spinnaker include path
FIND_PATH(SPINNAKER_INCLUDE_DIRS Spinnaker.h
  # Windows:
  "C:/Program Files/FLIR Systems/Spinnaker/include"
  "C:/Program Files (x86)/FLIR Systems/Spinnaker/include"
  "$ENV{VMLibraries_DIR}/extern/win${CMAKE_LIB_ARCH_APPENDIX}/include"
  # Linux
  "/usr/include/"
  "/usr/include/spinnaker/"
  "/usr/local/include/spinnaker/"
  "$ENV{VMLibraries_DIR}/extern/linux/include"
)

FIND_LIBRARY(SPINNAKER_LIBRARIES NAMES spinnaker Spinnaker Spinnaker_v140
    PATHS 
    "/usr/lib${CMAKE_LIB_ARCH_APPENDIX}"
    "C:/Program Files/FLIR Systems/Spinnaker"
    "C:/Program Files (x86)/FLIR Systems/Spinnaker"
    PATH_SUFFIXES ${SPINNAKER_POSSIBLE_LIB_DIRS}
)


IF(SPINNAKER_INCLUDE_DIRS AND SPINNAKER_LIBRARIES)
    SET(SPINNAKER_FOUND true)
ENDIF()

IF(SPINNAKER_FOUND)
    IF(NOT Spinnaker_FIND_QUIETLY)
        MESSAGE(STATUS "Found Spinnaker: ${SPINNAKER_LIBRARIES}")
    ENDIF(NOT Spinnaker_FIND_QUIETLY)
ELSE(SPINNAKER_FOUND)
    IF(Spinnaker_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find the Spinnaker library")
    ENDIF(Spinnaker_FIND_REQUIRED)
ENDIF(SPINNAKER_FOUND)
