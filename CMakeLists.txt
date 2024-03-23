cmake_minimum_required(VERSION 3.4.0)

if(NOT DEFINED PROJ_NAME)
	message(FATAL_ERROR "PROJ_NAME is not defined")
endif()
  
project(${PROJ_NAME} LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 20)

configure_file(${CMAKE_SOURCE_DIR}/src/utils/config/RootDir.h.in ${CMAKE_SOURCE_DIR}/src/utils/config/RootDir.h)

# add_subdirectory(dep/)

# add_subdirectory(src/)

set(target_LINK_LIBRARIES)
find_package(efsw CONFIG REQUIRED)
list(APPEND target_LINK_LIBRARIES efsw::efsw)

find_package(EnTT CONFIG REQUIRED)
list(APPEND target_LINK_LIBRARIES EnTT::EnTT)

find_package(glfw3 CONFIG REQUIRED)
list(APPEND target_LINK_LIBRARIES glfw)

find_package(glm CONFIG REQUIRED)
list(APPEND target_LINK_LIBRARIES glm::glm)

find_package(imgui CONFIG REQUIRED)
list(APPEND target_LINK_LIBRARIES imgui::imgui)

find_package(implot CONFIG REQUIRED)
list(APPEND target_LINK_LIBRARIES implot::implot)

find_package(unofficial-shaderc CONFIG REQUIRED)
list(APPEND target_LINK_LIBRARIES unofficial::shaderc::shaderc)

find_package(spdlog CONFIG REQUIRED)
list(APPEND target_LINK_LIBRARIES spdlog::spdlog)

find_package(Stb REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(tomlplusplus REQUIRED IMPORTED_TARGET tomlplusplus)
list(APPEND target_LINK_LIBRARIES PkgConfig::tomlplusplus)

find_package(volk CONFIG REQUIRED)
list(APPEND target_LINK_LIBRARIES volk::volk volk::volk_headers)

find_package(VulkanHeaders CONFIG)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
list(APPEND Vulkan::Headers GPUOpen::VulkanMemoryAllocator)

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

add_executable(${PROJ_NAME}
	main.cpp	
)

target_link_libraries(${PROJ_NAME} PRIVATE ${target_LINK_LIBRARIES})
