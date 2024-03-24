add_library(src-app-context STATIC
    VulkanApplicationContext.cpp
    context-creators/DeviceCreator.cpp
    context-creators/InstanceCreator.cpp
    context-creators/SurfaceCreator.cpp
    context-creators/SwapchainCreator.cpp
)

target_include_directories(src-app-context PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(src-app-context PRIVATE src-utils-logger volk::volk volk::volk_headers)
