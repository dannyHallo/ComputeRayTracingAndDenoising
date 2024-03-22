cmake_minimum_required(VERSION 3.4.0)

if(NOT DEFINED PROJ_NAME)
	message(FATAL_ERROR "PROJ_NAME is not defined")
endif()
  
project(${PROJ_NAME} LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 20)

configure_file(${CMAKE_SOURCE_DIR}/src/utils/config/RootDir.h.in ${CMAKE_SOURCE_DIR}/src/utils/config/RootDir.h)

# include_directories(
# 	src
#   dep
#   dep/spdlog/include
# 	dep/glfw
# 	dep/glm
# 	dep/imgui
# 	dep/volk
# 	dep/stb
# 	dep/obj-loader
# 	dep/memory-allocator-hpp)

# add_executable(${PROJ_NAME}
# 	src/main.cpp
# 	src/Application.cpp
# 	src/AppContext/VulkanApplicationContext.cpp
# 	src/memory/Image.cpp
# 	src/memory/ImageUtils.cpp
# 	src/memory/Buffer.cpp
# 	src/render-context/RenderSystem.cpp
# 	src/tris-ray-tracing/Bvh.cpp
# 	src/tris-ray-tracing/RtScene.cpp
# 	src/pipeline/ComputeMaterial.cpp
# 	src/pipeline/Material.cpp
# 	src/scene/mesh.cpp
# 	src/utils/Camera.cpp
# 	src/utils/logger.cpp

# 	src/window/Window.cpp

# 	dep/imgui/backends/imgui_impl_glfw.cpp
#     dep/imgui/backends/imgui_impl_vulkan.cpp
# 	dep/imgui/imgui_demo.cpp
#     dep/imgui/imgui_draw.cpp
#     dep/imgui/imgui_tables.cpp
#     dep/imgui/imgui_widgets.cpp
#     dep/imgui/imgui.cpp
# 	)

# target_link_directories(${PROJ_NAME}
#     PRIVATE dep/glfw/Release)

# target_link_libraries(${PROJ_NAME} glfw3.lib)
