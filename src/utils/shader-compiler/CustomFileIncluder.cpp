#include "CustomFileIncluder.hpp"

#include "utils/io/ShaderFileReader.hpp"
#include "utils/logger/Logger.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace {
// compresses path like: /a/b/../c/d.glsl into
// /a/c/d.glsl

std::string _compressPath(std::string const &originalPath) {
  std::vector<std::string> pathStack;
  std::stringstream ss(originalPath);
  std::string token;

  while (std::getline(ss, token, '/')) {
    if (token == "" || token == ".") {
      // skip empty tokens and current directory markers
      continue;
    } else if (token == "..") {
      // go up one level if possible
      if (!pathStack.empty()) {
        pathStack.pop_back();
      }
    } else {
      // normal directory or file, add to stack
      pathStack.push_back(token);
    }
  }

  // reconstruct the path
  std::string compressedPath;
  for (const auto &part : pathStack) {
    compressedPath += "/" + part;
  }

  // for windows platform: if original path is not beginned with /, delete the first /
  if (originalPath[0] != '/') {
    compressedPath.erase(0, 1);
  }

  // if the path was empty, return root "/"
  return compressedPath.empty() ? "/" : compressedPath;
}
} // namespace

CustomFileIncluder::CustomFileIncluder(Logger *logger,
                                       std::function<void(std::string const &)> includeCallback)
    : _logger(logger), _includeCallback(includeCallback) {}

shaderc_include_result *MakeErrorIncludeResult(const char *message) {
  return new shaderc_include_result{"", 0, message, strlen(message)};
}

struct FileInfo {
  std::string fullPath;
  std::string content;
};

shaderc_include_result *CustomFileIncluder::GetInclude(const char *requested_source,
                                                       shaderc_include_type /*include_type*/,
                                                       const char * /*requesting_source*/,
                                                       size_t /*include_depth*/) {
  std::string fullPath = _includeDir + requested_source;
  fullPath             = _compressPath(fullPath);

  if (_includeCallback != nullptr) {
    _includeCallback(fullPath);
  }

  std::string content = ShaderFileReader::readShaderSourceCode(fullPath, _logger);
  // store the pointer created in a pointer for destroying later on, eww!
  auto *info = new FileInfo{fullPath, content};
  return new shaderc_include_result{fullPath.c_str(), fullPath.size(), info->content.c_str(),
                                    info->content.length(), info};
}

void CustomFileIncluder::ReleaseInclude(shaderc_include_result *include_result) {
  auto *info = static_cast<FileInfo *>(include_result->user_data);
  delete info;
  delete include_result;
}
