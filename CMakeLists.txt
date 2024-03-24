cmake_minimum_required(VERSION 3.4.0)

project(voxel-lab LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 20)

configure_file(${CMAKE_SOURCE_DIR}/src/utils/config/RootDir.h.in ${CMAKE_SOURCE_DIR}/src/utils/config/RootDir.h)

# vcpkg
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
list(APPEND target_LINK_LIBRARIES Vulkan::Headers GPUOpen::VulkanMemoryAllocator)

set(vcpkg_INCLUDE_DIR "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/")

# configure my libraries
add_subdirectory(src/)

# configure executables using my libraries
add_subdirectory(apps/)

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
# 	testing.cpp	
# )

# set(target_INCLUDE_DIRECTORIES)
# list(APPEND target_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/src/)
# target_include_directories(${PROJ_NAME} PRIVATE ${target_INCLUDE_DIRECTORIES})
# target_link_libraries(${PROJ_NAME} PRIVATE ${target_LINK_LIBRARIES})
