#pragma once

#include "shaderc/shaderc.hpp"

class Logger;

class CustomFileIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
  CustomFileIncluder(Logger *logger);

  shaderc_include_result *GetInclude(const char *requested_source, shaderc_include_type type,
                                     const char *requesting_source, size_t include_depth) override;

  // for memory release of raw pointers
  void ReleaseInclude(shaderc_include_result *include_result) override;

private:
  Logger *_logger;
};
