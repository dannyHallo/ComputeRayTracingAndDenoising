cmake_minimum_required(VERSION 3.8)
project("${PROJ_NAME}")
set (CMAKE_CXX_STANDARD 17)

if(NOT DEFINED PROJ_NAME)
  set(PROJ_NAME "DefaultProjectName")
endif()

# Configure assets header file
configure_file(${CMAKE_SOURCE_DIR}/src/utils/RootDir.h.in ${CMAKE_SOURCE_DIR}/src/utils/RootDir.h)

include_directories(
	src
    dep
    dep/spdlog/include
	dep/glfw
	dep/glm
	dep/imgui
	dep/volk
	dep/stb
	dep/obj-loader
	dep/memory-allocator-hpp)

add_executable(${PROJ_NAME}
	src/app-context/VulkanApplicationContext.cpp
	src/main.cpp
	src/Application.cpp
	src/memory/Image.cpp
	src/memory/ImageUtils.cpp
	src/memory/Buffer.cpp
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
	src/utils/logger.cpp

	dep/imgui/backends/imgui_impl_glfw.cpp
    dep/imgui/backends/imgui_impl_vulkan.cpp
	dep/imgui/imgui_demo.cpp
    dep/imgui/imgui_draw.cpp
    dep/imgui/imgui_tables.cpp
    dep/imgui/imgui_widgets.cpp
    dep/imgui/imgui.cpp
	)

target_link_directories(${PROJ_NAME}
    PRIVATE dep/glfw/Release)

target_link_libraries(${PROJ_NAME} glfw3.lib)
