add_library(src-imgui-manager STATIC
    gui-elements/FpsGui.cpp
    gui-manager/ImguiManager.cpp
    imgui-backends/imgui_impl_glfw.cpp
    imgui-backends/imgui_impl_vulkan.cpp
)

target_include_directories(src-imgui-manager PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(src-imgui-manager PRIVATE
    src-utils-logger
    src-app-context
    src-config-container
    src-utils-fps-sink
    src-window
    glfw
    imgui::imgui
    implot::implot
)
