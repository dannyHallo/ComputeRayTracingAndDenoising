#pragma once

#include <fstream>
#include <string>
#include <vector>

namespace ShaderFileReader {
inline std::vector<char> readShaderFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("shader file: " + filename + " not found");
  }
  std::streamsize fileSize = (std::streamsize)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
  file.close();
  return buffer;
}
}; // namespace ShaderFileReader