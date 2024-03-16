#pragma system_header

#pragma once

// disable exceptions for clang compiler
#define TOML_EXCEPTIONS 0
// disable header only mode for faster compilation
#define TOML_HEADER_ONLY 0

#include "toml++/toml.hpp" // IWYU pragma: export