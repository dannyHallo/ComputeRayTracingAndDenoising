add_library(src-config-container STATIC
    sub-config/ApplicationInfo.cpp
    sub-config/AtmosInfo.cpp
    sub-config/BrushInfo.cpp
    sub-config/CameraInfo.cpp
    sub-config/DebugInfo.cpp
    sub-config/ImguiManagerInfo.cpp
    sub-config/PostProcessingInfo.cpp
    sub-config/ShadowMapCameraInfo.cpp
    sub-config/SvoTracerInfo.cpp
    sub-config/TerrainInfo.cpp
    sub-config/TracingInfo.cpp
    ConfigContainer.cpp
)

target_include_directories(src-config-container PRIVATE
    ${vcpkg_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src/
)

target_link_libraries(src-config-container PRIVATE
    src-utils-logger
    src-utils-toml-config
    glm::glm
)
