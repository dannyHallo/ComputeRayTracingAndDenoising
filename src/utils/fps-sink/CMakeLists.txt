add_library(src-utils-fps-sink STATIC FpsSink.cpp MovingAvg.cpp)
target_include_directories(src-utils-fps-sink PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)
