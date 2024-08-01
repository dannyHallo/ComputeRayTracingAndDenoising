#pragma once

#include <glm/glm.hpp>

class TomlConfigReader;

struct CameraInfo {
  glm::vec3 initPosition{};
  float initYaw{};   // in euler angles
  float initPitch{}; // in euler angles
  float vFov{};
  float movementSpeed{};
  float movementSpeedBoost{};
  float mouseSensitivity{};

  void loadConfig(TomlConfigReader *tomlConfigReader);
};
