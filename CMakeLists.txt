cmake_minimum_required(VERSION 2.8)
include(${CMAKE_SOURCE_DIR}/cmake/include/define_module.cmake)

#######################
# project information #
#######################

project(yanta)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)


###############
# config file #
###############

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/config.h.in
  ${PROJECT_BINARY_DIR}/config.h
)

#######################
# project directories #
#######################

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/modules)
include_directories(${PROJECT_SOURCE_DIR}/modules/yanta)

#####################
# compiler settings #
#####################

set(SKIA_DEBUG_FLAGS "-DSK_INTERNAL -DSK_GAMMA_SRGB -DSK_GAMMA_APPLY_TO_A8 -DSK_SCALAR_TO_FLOAT_EXCLUDED -DSK_ALLOW_STATIC_GLOBAL_INITIALIZERS=1 -DSK_SUPPORT_GPU=1 -DSK_SUPPORT_OPENCL=0 -DSK_FORCE_DISTANCEFIELD_FONTS=0 -DSK_PICTURE_USE_SK_RECORD -DSK_PICTURE_OPTIMIZE_SK_RECORD -DSK_SCALAR_IS_FLOAT -DSK_SAMPLES_FOR_X -DSK_BUILD_FOR_UNIX -DSK_USE_POSIX_THREADS -DSK_DEVELOPER=1")
set(SKIA_RELEASE_FLAGS "-DSK_INTERNAL -DSK_GAMMA_SRGB -DSK_GAMMA_APPLY_TO_A8 -DSK_SCALAR_TO_FLOAT_EXCLUDED -DSK_ALLOW_STATIC_GLOBAL_INITIALIZERS=1 -DSK_SUPPORT_GPU=1 -DSK_SUPPORT_OPENCL=0 -DSK_FORCE_DISTANCE_FIELD_TEXT=0 -DSK_SCALAR_IS_FLOAT -DSK_SAMPLES_FOR_X -DSK_BUILD_FOR_UNIX -DSK_USE_POSIX_THREADS -DNDEBUG")

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -DBOOST_DISABLE_ASSERTS -Wall -Wextra -fomit-frame-pointer ${SKIA_RELEASE_FLAGS}")
set(CMAKE_CXX_FLAGS_PROFILING "-O3 -DNDEBUG -DBOOST_DISABLE_ASSERTS -g -fno-omit-frame-pointer -Wall -Wextra ${SKIA_RELEASE_FLAGS}" CACHE STRING "Profiling CXX flags" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG   "-g -Wall -Wextra ${SKIA_DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
mark_as_advanced(CMAKE_CXX_FLAGS_PROFILING)

##################
# subdirectories #
##################

add_subdirectory(modules/util)
add_subdirectory(modules/signals)
add_subdirectory(modules/pipeline)
add_subdirectory(modules/gui)
add_subdirectory(modules/yanta)

###############
# main target #
###############

define_module(yanta BINARY SOURCES yanta.cpp LINKS yantalib)
define_module(benchmarks BINARY SOURCES benchmarks.cpp LINKS yantalib)
#define_module(tests BINARY SOURCES tests.cpp LINKS yantalib)
define_module(test_skia BINARY SOURCES test_skia.cpp LINKS yantalib)

