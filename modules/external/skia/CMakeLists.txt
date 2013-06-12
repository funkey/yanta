# find all files constituting this library
file(GLOB skiafiles
  src/core/*.cpp
  src/image/*.cpp
  src/opts/*.cpp
  src/pathops/*.cpp
  src/pdf/*.cpp
  src/ports/*.cpp
  src/utils/*.cpp
  src/pipe/*.cpp)

add_definitions(-DSK_SUPPORT_GPU=0)

include_directories(include/core)
include_directories(include/config)
include_directories(include/gpu)
include_directories(include/utils)
include_directories(include/pathops)
include_directories(include/ports)
include_directories(include/pdf)
include_directories(include/xml)
include_directories(include/lazy)
include_directories(include/images)
include_directories(include/effects)
include_directories(include/pipe)
include_directories(src/gpu)
include_directories(src/image)
include_directories(src/core)
include_directories(src/pdf)
include_directories(src/utils)
include_directories(src/lazy)

# this is a library
add_library(skia ${skiafiles})
