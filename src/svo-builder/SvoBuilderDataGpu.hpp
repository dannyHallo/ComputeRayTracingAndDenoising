#pragma once

#include "utils/incl/Glm.hpp" // IWYU pragma: export

#include <cstdint> // IWYU pragma: export

// alignment rule:
// https://www.oreilly.com/library/view/opengl-programming-guide/9780132748445/app09lev1sec3.html

// basically,
// scalar types are default aligned
// two component vectors are aligned twice the size of the component type
// three and four component vectors are aligned four times the size of the component type

#define vec3 alignas(16) glm::vec3
#define uvec3 alignas(16) glm::uvec3
#define vec2 alignas(8) glm::vec2
#define mat4 alignas(16) glm::mat4
#define uvec2 alignas(8) glm::uvec2
#define bool alignas(4) bool
#define uint uint32_t

#include "shaders/include/svoBuilderDataStructs.glsl" // IWYU pragma: export

#undef vec3
#undef uvec3
#undef vec2
#undef mat4
#undef uvec2
#undef bool
#undef uint
