cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

#######################
# project information #
#######################

project(yanta)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

#####################
# required packages #
#####################

message(STATUS ${CMAKE_MODULE_PATH})

find_package(Boost 1.42 COMPONENTS date_time filesystem program_options signals system thread timer REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIR})
  message(STATUS "Boost found.")
else()
  message(STATUS "Boost *NOT* found.")
endif()

find_package(X11 REQUIRED)
if (X11_Xrandr_FOUND)
  include_directories(${X11_INCLUDE_DIR})
  message(STATUS "Xrandr found")
else()
  message(STATUS "Xrandr *NOT* found")
endif()

find_package(OpenGL)
if(OPENGL_FOUND)
  message(STATUS "OpenGL found.")
else()
  message(STATUS "OpenGL *NOT* found.")
endif()

find_package(GLEW)
if (GLEW_FOUND)
  include_directories(${GLEW_INCLUDE_DIR})
  message(STATUS "GLEW found.")
else()
  message(STATUS "GLEW *NOT* found.")
endif()

find_package(GLUT)
if (GLUT_FOUND)
  include_directories(${GLUT_INCLUDE_DIR})
  message(STATUS "GLUT found.")
else()
  message(STATUS "GLUT *NOT* found.")
endif()

find_package(Freetype)
if (FREETYPE_FOUND)
  message(STATUS "Freetype found.")
else()
  message(STATUS "Freetype *NOT* found.")
endif()

find_package(Skia)
if (SKIA_FOUND)
  message(STATUS "Skia found.")
  set(HAVE_SKIA 1)
else()
  message(STATUS "Skia not found -- will download and build it on demand.")
  include(ExternalProject)
  ExternalProject_Add(
    skia
    GIT_REPOSITORY gitolite@lego.slyip.net:fun/skia
    GIT_TAG 3f9a7e4bfafd8d2f0fef98470adcbbd8f1a2204c
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    INSTALL_COMMAND ""
  )
  ExternalProject_Get_Property(skia SOURCE_DIR)
  SET(Skia_INCLUDE_DIR ${SOURCE_DIR}/include)
  SET(Skia_LIBRARY ${SOURCE_DIR}/out/Release/libskia.a)
  set(HAVE_SKIA 1)
  set(OWN_SKIA 1)
endif()
include_directories(${Skia_INCLUDE_DIR}/config)
include_directories(${Skia_INCLUDE_DIR}/core)
include_directories(${Skia_INCLUDE_DIR}/effects)
include_directories(${Skia_INCLUDE_DIR}/pdf)

#####################
# optional packages #
#####################

find_package(Doxygen)
if (DOXYGEN_FOUND)
  configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/doxygen.in
  ${PROJECT_BINARY_DIR}/doxygen.conf
  )
  add_custom_target(
  doc
  ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/doxygen.conf
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  COMMENT "Generating Doxygen documentation"
  VERBATIM
  )
  message(STATUS "Doxygen found.")
else()
  message(STATUS "Doxygen *NOT* found.")
endif()

###############
# config file #
###############

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/config.h.in
  ${PROJECT_BINARY_DIR}/config.h
)

############
# all libs #
############

set(CORELIBS
  ${X11_X11_LIB}
  ${X11_Xrandr_LIB}
  ${X11_Xinput_LIB}
  ${OPENGL_LIBRARY}
  ${GLEW_LIBRARY}
  ${GLUT_LIBRARIES}
  ${Boost_LIBRARIES}
  ${Cairo_LIBRARY}
  ${Skia_LIBRARY}
  ${FREETYPE_LIBRARIES}
  -lz
  -lfontconfig
)

#######################
# project directories #
#######################

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/modules)
include_directories(${PROJECT_SOURCE_DIR}/yanta)

add_subdirectory(modules/util)
add_subdirectory(modules/signals)
add_subdirectory(modules/pipeline)
add_subdirectory(modules/gui)

#####################
# compiler settings #
#####################

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall -Wextra -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_PROFILING "-O3 -DNDEBUG -g -fno-omit-frame-pointer -Wall -Wextra" CACHE STRING "Profiling CXX flags" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG   "-g -DNDEBUG -Wall -Wextra")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
mark_as_advanced(CMAKE_CXX_FLAGS_PROFILING)

###############
# main target #
###############

file (GLOB_RECURSE yantafiles yanta/*.cpp)

add_executable(yanta yanta.cpp ${yantafiles})
target_link_libraries(yanta signals pipeline gui util ${CORELIBS})

if (OWN_SKIA)
  add_dependencies(yanta skia)
endif()
