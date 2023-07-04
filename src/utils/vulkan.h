#pragma once

// https://github.com/zeux/volk

// volk is a dynamic loader for vulkan applications, it does not need to link to static libraries (vulkan-1.lib)
// instead, it loads the vulkan functions at runtime, using dynamic libraries (vulkan-1.dll), which is installed
// at Windows/System32 (by the vulkan sdk installer?)

// volk reduces overhead by avoiding loading windows.h when VK_USE_PLATFORM_WIN32_KHR is defined
// #define VK_USE_PLATFORM_WIN32_KHR -> this is not required, since we are not using platform related vulkan functions
#include "volk.h"