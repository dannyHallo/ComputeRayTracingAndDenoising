add_library(src-application STATIC
    svo-builder/SvoBuilder.cpp
    svo-tracer/SvoTracer.cpp
    Application.cpp
)

target_include_directories(src-application PRIVATE
    ${vcpkg_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/resources/shaders/include/
    ${CMAKE_SOURCE_DIR}/dep/opengametools/src/
)

target_link_libraries(src-application PRIVATE
    src-camera
    src-window
    src-imgui-manager
    src-app-context
    src-file-watcher
    src-config-container
    src-utils-io
    src-utils-logger
    src-utils-fps-sink
    src-utils-shader-compiler
    src-custom-mem-alloc
    src-vulkan-wrapper
    glm::glm
)
