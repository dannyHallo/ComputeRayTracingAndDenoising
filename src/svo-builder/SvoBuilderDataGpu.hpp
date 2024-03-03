#pragma once

#include "utils/incl/Glm.hpp" // IWYU pragma: export

#include <cstdint> // IWYU pragma: export

#define uint uint32_t
#define uvec3 glm::uvec3

#include "shaders/include/svoBuilderDataStructs.glsl" // IWYU pragma: export

#undef uint
#undef uvec3
