add_library(src-file-watcher STATIC ShaderChangeListener.cpp)
target_include_directories(src-file-watcher PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(src-file-watcher PRIVATE 
    efsw::efsw
    src-utils-logger
    src-utils-event-dispatcher
    src-vulkan-wrapper
)
