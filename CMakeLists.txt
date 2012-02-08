# CMake build script for RFA broadcast provider
# x64 Windows Server-only
# 2011/11/30 -- Steven.McCoy@thomsonreuters.com

cmake_minimum_required (VERSION 2.8)

project (Nezumi)

set(RFA_ROOT C:/rfa7.2.0.L1.win-shared.rrg)
set(BOOST_ROOT C:/boost_1_48_0)
set(BOOST_LIBRARYDIR ${BOOST_ROOT}/stage/lib)

set(Boost_USE_STATIC_LIBS ON)

find_package (Boost 1.44 COMPONENTS thread REQUIRED)

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
	-DWIN32
	-DWIN32_LEAN_AND_MEAN
# Windows Server 2008 (not-R2)
	-D_WIN32_WINNT=0x0600
)

#-----------------------------------------------------------------------------
# source files

set(cxx-sources
	src/config.cc
	src/error.cc
	src/main.cc
	src/nezumi.cc
	src/provider.cc
	src/rfa.cc
	src/rfa_logging.cc
	src/chromium/chromium_switches.cc
	src/chromium/command_line.cc
	src/chromium/debug/stack_trace.cc
	src/chromium/debug/stack_trace_win.cc
	src/chromium/memory/singleton.cc
	src/chromium/logging.cc
	src/chromium/string_piece.cc
	src/chromium/string_split.cc
	src/chromium/string_util.cc
	src/chromium/synchronization/lock.cc
	src/chromium/synchronization/lock_impl_win.cc
	src/chromium/vlog.cc
)

include_directories(
	include
	${RFA_ROOT}/Include
	${RFA_ROOT}/Include/rwf
	${Boost_INCLUDE_DIRS}
)

link_directories(
	${RFA_ROOT}/Libs/WIN_64_VS100/Release_MD
	${Boost_LIBRARY_DIRS}
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
	${Boost_LIBRARIES}
	ws2_32.lib
	dbghelp.lib
)

# end of file
