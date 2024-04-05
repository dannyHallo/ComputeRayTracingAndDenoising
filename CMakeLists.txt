# cmake guidelines: https://cliutils.gitlab.io/modern-cmake/

cmake_minimum_required(VERSION 3.5.0)

project(voxel-lab)

if(WITH_PORTABLE_RESOURCES)
    message(STATUS "Portable resources folder is used")
    add_definitions(-DPORTABLE_RESOURCES_FOLDER)
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL release)
    add_definitions(-DNVALIDATIONLAYERS)
endif()

configure_file(${CMAKE_SOURCE_DIR}/src/utils/config/RootDir.h.in ${CMAKE_SOURCE_DIR}/src/utils/config/RootDir.h)

set(target_LINK_LIBRARIES)

# vcpkg package loading
# efsw::efsw
find_package(efsw CONFIG REQUIRED)

# EnTT::EnTT
find_package(EnTT CONFIG REQUIRED)

# glfw
find_package(glfw3 CONFIG REQUIRED)

# glm::glm
find_package(glm CONFIG REQUIRED)

# imgui::imgui
find_package(imgui CONFIG REQUIRED)

# implot::implot
find_package(implot CONFIG REQUIRED)

# unofficial::shaderc::shaderc
find_package(unofficial-shaderc CONFIG REQUIRED)

# spdlog::spdlog
find_package(spdlog CONFIG REQUIRED)

find_package(Stb REQUIRED)

# PkgConfig::tomlplusplus
find_package(PkgConfig REQUIRED)

# pkg_check_modules(tomlplusplus REQUIRED IMPORTED_TARGET tomlplusplus)

# volk::volk volk::volk_headers
find_package(volk CONFIG REQUIRED)

# Vulkan::Headers GPUOpen::VulkanMemoryAllocator
find_package(VulkanHeaders CONFIG)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

set(vcpkg_INCLUDE_DIR "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/")

add_subdirectory(src/)
add_subdirectory(apps/)

# copy compile_commands.json to .vscode folder for clangd to use, this is because the build dir for debug and release mode is different
add_custom_target(copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/.vscode
)
