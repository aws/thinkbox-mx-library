# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required( VERSION 3.20 FATAL_ERROR )

project( ThinkboxMXLibrary )

find_package( thinkboxcmlibrary REQUIRED )
include( PrecompiledHeader )
include( ThinkboxCMLibrary )

set( SUPPORTED_MAX_VERSIONS 2022 2023 )

option( MAX_VERSION "The version of 3ds Max SDK to build the library against." 2022 )

if( NOT MAX_VERSION IN_LIST SUPPORTED_MAX_VERSIONS )
  message( FATAL_ERROR "ERROR: Cannot build for unsupported 3ds Max version ${MAX_VERSION}" )
endif()

add_library( thinkboxmxlibrary STATIC )

set_property( TARGET thinkboxmxlibrary PROPERTY CXX_STANDARD 17 )

target_include_directories( thinkboxmxlibrary PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

file( GLOB_RECURSE H_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "frantic/*.h"
  "frantic/*.hpp"
)

file( GLOB_RECURSE CXX_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "src/*.cpp"
)
list( REMOVE_ITEM CXX_FILES "src/particles/tp_particle_istream70.cpp" )

target_sources( thinkboxmxlibrary PRIVATE
	stdafx.cpp
	stdafx.h
  ${H_FILES}
  ${CXX_FILES}
)

# The Conan version of Boost was built with this, and it changes the library names.
# As a result, we need to set this to tell Boost to look for the right libraries to
# link against.
target_compile_definitions( thinkboxmxlibrary PUBLIC BOOST_AUTO_LINK_SYSTEM )

find_package( thinkboxlibrary REQUIRED )
find_package( Boost REQUIRED )
find_package( OpenEXR REQUIRED )
find_package( ZLIB REQUIRED )
find_package( TBB REQUIRED )
find_package( tinyxml2 REQUIRED )
find_package( maxsdk REQUIRED )
# find_package( phoenex_fd_sdk REQUIRED )
# find_package( thinking_particles_light_line_sdk REQUIRED )

target_include_directories( thinkboxmxlibrary PUBLIC ${thinkboxlibrary_INCLUDE_DIRS} )
target_include_directories( thinkboxmxlibrary PUBLIC ${Boost_INCLUDE_DIRS} )
target_include_directories( thinkboxmxlibrary PUBLIC ${OpenEXR_INCLUDE_DIRS} )
target_include_directories( thinkboxmxlibrary PUBLIC ${ZLIB_INCLUDE_DIRS} )
target_include_directories( thinkboxmxlibrary PUBLIC ${TBB_INCLUDE_DIRS} )
target_include_directories( thinkboxmxlibrary PUBLIC ${tinyxml2_INCLUDE_DIRS} )
target_include_directories( thinkboxmxlibrary PUBLIC ${maxsdk_INCLUDE_DIR} )

target_link_libraries( thinkboxmxlibrary INTERFACE thinkboxlibrary::thinkboxlibrary )
target_link_libraries( thinkboxmxlibrary INTERFACE Boost::Boost )
target_link_libraries( thinkboxmxlibrary INTERFACE OpenEXR::OpenEXR )
target_link_libraries( thinkboxmxlibrary INTERFACE ZLIB::ZLIB )
target_link_libraries( thinkboxmxlibrary INTERFACE TBB::tbb )
target_link_libraries( thinkboxmxlibrary INTERFACE tinyxml2::tinyxml2 )
target_link_libraries( thinkboxmxlibrary INTERFACE maxsdk::maxsdk )

frantic_common_platform_setup( thinkboxmxlibrary )
frantic_default_source_groups( thinkboxmxlibrary HEADERDIR include SOURCEDIR src )

# Disable optimization for the RelWithDebInfo configuration on Windows.
# This allows breakpoints to be hit reliably when debugging in Visual Studio.
if( WIN32 )
	target_compile_options( thinkboxmxlibrary PRIVATE "$<$<CONFIG:Release>:/O2>$<$<CONFIG:RelWithDebInfo>:/Od>" )
endif()

# add_library( tp_70_sdk_wrapper STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/particles/tp_particle_istream70.cpp" )
# target_compile_definitions( tp_70_sdk_wrapper PRIVATE FRANTIC_MAX_SKIP_PCH )
# frantic_import_library( tp_70_sdk_wrapper franticlib FINDNAME FranticLib )
# frantic_import_library( tp_70_sdk_wrapper Boost )
# frantic_import_library( tp_70_sdk_wrapper max_sdk )
# frantic_import_library( tp_70_sdk_wrapper thinking_particles_sdk_7_0 )
# frantic_import_library( tp_70_sdk_wrapper thinking_particles_light_line_sdk )
# frantic_import_library( tp_70_sdk_wrapper OpenEXR )
# target_include_directories( tp_70_sdk_wrapper PUBLIC
#                             $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
#                             $<INSTALL_INTERFACE:include> )
# frantic_common_platform_setup( tp_70_sdk_wrapper )
# frantic_default_source_groups( tp_70_sdk_wrapper HEADERDIR include SOURCEDIR src )

install( DIRECTORY frantic
          DESTINATION include
          FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
)
install( TARGETS thinkboxmxlibrary
          RUNTIME DESTINATION bin
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib
)
