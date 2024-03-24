add_library(src-window STATIC Window.cpp)
target_include_directories(src-window PRIVATE ${vcpkg_INCLUDE_DIR})
target_link_libraries(src-window PRIVATE glfw volk::volk volk::volk_headers)