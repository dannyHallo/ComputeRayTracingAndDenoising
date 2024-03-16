#pragma system_header

#pragma once

#ifndef CUSTOM_IMGUI_IMPLEMENTATION
#include "imgui.h"                            // IWYU pragma: export
#include "imgui/backends/imgui_impl_glfw.h"   // IWYU pragma: export
#include "imgui/backends/imgui_impl_vulkan.h" // IWYU pragma: export
#include "implot.h"                           // IWYU pragma: export
#else
#include "imgui/backends/imgui_impl_glfw.cpp"   // IWYU pragma: export
#include "imgui/backends/imgui_impl_vulkan.cpp" // IWYU pragma: export
#endif
