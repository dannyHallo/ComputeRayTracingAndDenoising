#pragma once

#include "glm/glm.hpp" // IWYU pragma: export

class TomlConfigReader;

struct AtmosInfo {
  float sunAltitude{};
  float sunAzimuth{};
  glm::vec3 rayleighScatteringBase{};
  float mieScatteringBase{};
  float mieAbsorptionBase{};
  glm::vec3 ozoneAbsorptionBase{};
  float sunLuminance{};
  float atmosLuminance{};
  float sunSize{};
  float rayleighDropoffFac{};
  float mieDropoffFac{};

  void loadConfig(TomlConfigReader *tomlConfigReader);
};
