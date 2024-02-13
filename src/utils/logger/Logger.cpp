#define SPDLOG_HEADER_ONLY
#include "Logger.hpp"

// this is required to define format functions that are used by spdlog
#define FMT_HEADER_ONLY
#include "spdlog/fmt/bundled/format.h"

#include "spdlog/spdlog.h" // spdlog/include/spdlog/details/windows_include.h defines APIENTRY!
