add_library(src-utils-shader-compiler CustomFileIncluder.cpp ShaderCompiler.cpp)
target_include_directories(src-utils-shader-compiler PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(src-utils-shader-compiler PRIVATE 
    src-utils-logger
    src-utils-io
    unofficial::shaderc::shaderc
)
