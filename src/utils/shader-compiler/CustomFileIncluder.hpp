#pragma once

#include "shaderc/shaderc.hpp"

#include <functional>
#include <string>

class Logger;

class CustomFileIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
  CustomFileIncluder(Logger *logger,
                     std::function<void(std::string const &)> includeCallback = nullptr);

  shaderc_include_result *GetInclude(const char *requested_source, shaderc_include_type type,
                                     const char *requesting_source, size_t include_depth) override;

  // for memory release of raw pointers
  void ReleaseInclude(shaderc_include_result *include_result) override;

  // custom function can be added here
  void setIncludeDir(const std::string &includeDir) { _includeDir = includeDir; }

private:
  Logger *_logger;
  std::function<void(std::string const &)> _includeCallback;

  std::string _includeDir{};
};
