cmake_minimum_required(VERSION 3.8)
project(vulkan)
set (CMAKE_CXX_STANDARD 17)

# # find source files using globbing
# file(GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_SOURCE_DIR}
# 	src/*.c
# 	src/*.cpp)
# # printing
# message(STATUS ${SOURCE_FILES})

# Configure assets header file
configure_file(${CMAKE_SOURCE_DIR}/src/utils/RootDir.h.in ${CMAKE_SOURCE_DIR}/src/utils/RootDir.h)

# Build dependencies
add_subdirectory(dep/glfw)

include_directories(
	src
    dep
    dep/spdlog/include
	dep/glfw/include
	dep/glm
	dep/imgui
	dep/volk
	dep/stb
	dep/obj-loader
	dep/memory-allocator-hpp)

add_executable(${PROJECT_NAME}
	src/app-context/VulkanApplicationContext.cpp
	src/main.cpp
	src/Application.cpp
	src/memory/Image.cpp
	src/memory/ImageUtils.cpp
	src/render-context/FlatRenderPass.cpp
	src/render-context/ForwardRenderPass.cpp
	src/render-context/RenderSystem.cpp
	src/ray-tracing/Bvh.cpp
	src/ray-tracing/RtScene.cpp
	src/scene/ComputeMaterial.cpp
	src/scene/ComputeModel.cpp
	src/scene/DrawableModel.cpp
	src/scene/Material.cpp
	src/scene/Scene.cpp
	src/scene/mesh.cpp
	src/utils/Camera.cpp
	src/utils/Window.cpp
	src/utils/StbImageImpl.cpp

	dep/imgui/backends/imgui_impl_glfw.cpp
    dep/imgui/backends/imgui_impl_vulkan.cpp
	dep/imgui/imgui_demo.cpp
    dep/imgui/imgui_draw.cpp
    dep/imgui/imgui_tables.cpp
    dep/imgui/imgui_widgets.cpp
    dep/imgui/imgui.cpp
	)

target_link_directories(${PROJECT_NAME}
    PRIVATE ${PROJECT_BINARY_DIR}/dep/glfw/src/Release)

target_link_libraries(${PROJECT_NAME} glfw3.lib)
