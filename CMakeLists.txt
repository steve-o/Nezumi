# CMake build script for RFA broadcast provider
# x64 Windows Server-only
# 2011/11/30 -- Steven.McCoy@thomsonreuters.com

cmake_minimum_required (VERSION 2.8)

project (Nezumi)

set(RFA_ROOT C:/rfa7.2.0.L1.win-shared.rrg)
set(BOOST_ROOT C:/boost_1_48_0)

find_package (Boost 1.44 REQUIRED)

#-----------------------------------------------------------------------------
# force off-tree build

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
message(FATAL_ERROR "CMake generation is not allowed within the source directory!
Remove the CMakeCache.txt file and try again from another folder, e.g.:

   del CMakeCache.txt
   mkdir build
   cd build
   cmake ..
")
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

#-----------------------------------------------------------------------------
# default to Release build

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR}/lib)

#-----------------------------------------------------------------------------
# platform specifics

add_definitions(
)

#-----------------------------------------------------------------------------
# source files

set(cxx-sources
	src/nezumi.cc
	src/config.cc
	src/rfa.cc
	src/log.cc
	src/event_queue.cc
	src/provider.cc
	src/main.cc
	src/error.cc
	${RFA_ROOT}/Examples/StarterCommon/AppUtil.cpp
	${RFA_ROOT}/Examples/StarterCommon/CtrlBreakHandler.cpp
	${RFA_ROOT}/Examples/StarterCommon/Encoder.cpp
	${RFA_ROOT}/Examples/StarterCommon/ExceptionHandler.cpp
	${RFA_ROOT}/Examples/StarterCommon/OMMStrings.cpp
	${RFA_ROOT}/Examples/StarterCommon/Timer.cpp
)

include_directories(
	include
	${RFA_ROOT}/Include
	${RFA_ROOT}/Include/rwf
	${RFA_ROOT}/Examples
	${Boost_INCLUDE_DIRS}
)

link_directories(
	${RFA_ROOT}/Libs/WIN_64_VS100/Release_MD
)

#-----------------------------------------------------------------------------
# output

add_executable(Nezumi ${cxx-sources})

target_link_libraries(Nezumi
	RFA7_Common100_x64.lib
	RFA7_Config100_x64.lib
	RFA7_Logger100_x64.lib
	RFA7_Data100_x64.lib
	RFA7_Connections100_x64.lib
	RFA7_Connections_OMM100_x64.lib
	RFA7_SessionLayer100_x64.lib
	ws2_32.lib
)

# end of file
