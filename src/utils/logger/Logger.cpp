#define SPDLOG_HEADER_ONLY
#include "Logger.hpp" // IWYU pragma: export

// this is required to define format functions that are used by spdlog
#define FMT_HEADER_ONLY
#include "spdlog/fmt/bundled/format.h" // IWYU pragma: export
