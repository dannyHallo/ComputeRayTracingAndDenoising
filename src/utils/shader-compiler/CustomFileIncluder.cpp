#include "CustomFileIncluder.hpp"

#include "utils/config/RootDir.h"
#include "utils/file-io/ShaderFileReader.hpp"
#include "utils/logger/Logger.hpp"

CustomFileIncluder::CustomFileIncluder(Logger *logger) : _logger(logger) {}

shaderc_include_result *MakeErrorIncludeResult(const char *message) {
  return new shaderc_include_result{"", 0, message, strlen(message)};
}

struct FileInfo {
  std::string fullPath;
  std::string contents;
};

shaderc_include_result *CustomFileIncluder::GetInclude(const char *requested_source,
                                                       shaderc_include_type /*include_type*/,
                                                       const char * /*requesting_source*/,
                                                       size_t /*include_depth*/) {
  std::string fullPath = kRootDir + "src/shaders/" + requested_source;
  std::string content  = ShaderFileReader::readShaderSourceCode(fullPath, _logger);
  return new shaderc_include_result{fullPath.c_str(), fullPath.size(), content.c_str(),
                                    content.size(), new FileInfo{fullPath, content}};
}

void CustomFileIncluder::ReleaseInclude(shaderc_include_result *include_result) {
  auto *info = static_cast<FileInfo *>(include_result->user_data);
  delete info;
  delete include_result;
}
