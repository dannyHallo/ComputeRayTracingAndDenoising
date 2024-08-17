#pragma once

#include "glm/glm.hpp" // IWYU pragma: export

class TomlConfigReader;

struct DebugInfo {
  bool debugB1{};
  float debugF1{};
  int debugI1{};
  glm::vec3 debugC1{};

  void loadConfig(TomlConfigReader *tomlConfigReader);
};
