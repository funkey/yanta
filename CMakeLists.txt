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

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -DBOOST_DISABLE_ASSERTS -DSK_RELEASE -Wall -Wextra -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_PROFILING "-O3 -DNDEBUG -DBOOST_DISABLE_ASSERTS -DSK_RELEASE -g -fno-omit-frame-pointer -Wall -Wextra" CACHE STRING "Profiling CXX flags" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG   "-g -DSK_RELEASE -Wall -Wextra")
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
