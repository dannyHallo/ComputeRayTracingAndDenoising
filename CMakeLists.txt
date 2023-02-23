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
add_subdirectory(dep/vk-bootstrap)

include_directories(
	src
    dep
	dep/glfw/include
	dep/glm
	dep/stb
	dep/obj-loader
	dep/memory-allocator-hpp
	dep/vk-bootstrap/src)

add_executable(${PROJECT_NAME}
	src/app-context/VulkanApplicationContext.cpp
	src/app-context/VulkanSwapchain.cpp
	src/main.cpp
	src/memory/Image.cpp
	src/render-context/FlatRenderPass.cpp
	src/render-context/ForwardRenderPass.cpp
	src/render-context/RenderSystem.cpp
	src/scene/ComputeMaterial.cpp
	src/scene/ComputeModel.cpp
	src/scene/DrawableModel.cpp
	src/scene/Material.cpp
	src/scene/Scene.cpp
	src/scene/mesh.cpp
	src/utils/StbImageImpl.cpp)

target_link_directories(${PROJECT_NAME}
    PRIVATE ${PROJECT_BINARY_DIR}/dep/glfw/src/Release
    PRIVATE ${PROJECT_BINARY_DIR}/dep/vk-bootstrap/Release
	PRIVATE $ENV{VULKAN_SDK}/Lib)

target_link_libraries(${PROJECT_NAME} glfw3.lib vk-bootstrap.lib vulkan-1.lib)
