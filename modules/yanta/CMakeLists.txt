file (GLOB_RECURSE yantafiles *.cpp)
add_library(yantalib OBJECT ${yantafiles})
add_dependencies(yantalib skia)
